#!/usr/bin/env python3
# /// script
# dependencies = [
#   "duckdb>=1.4,<2",
# ]
# ///
"""Convert BinPrey/x86-instruction-test-vectors parquet files.

The input repository stores one parquet file per CPU with rows shaped as:

    mnemonic, asm, encoding, inputs, outputs, exception

This script emits the pooled x86Tester text format consumed by remill-tester:

    data:<N>
    #<hex 0>
    #<hex 1>
    ...
    instr:<address>;#<opcode_hex>;<disassembly>;<row_count>;in=<schema>;out=<schema>
    <input-indexes>|<output-indexes-or-!exception>
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import OrderedDict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

import duckdb


DEFAULT_ADDRESS = 0x04000001
FETCH_CHUNK_SIZE = 8192


class ConversionError(RuntimeError):
    pass


@dataclass
class Group:
    encoding: str
    asm: str
    input_schema: tuple[str, ...]
    output_schema: tuple[str, ...]
    rows: list[str] = field(default_factory=list)


class OutputCorpus:
    def __init__(self, path: Path, *, address: int):
        self.path = path
        self.address = address
        self.pool_indexes: dict[str, int] = {}
        self.pool_values: list[str] = []
        self.groups: OrderedDict[
            tuple[str, str, tuple[str, ...], tuple[str, ...]], Group
        ] = OrderedDict()
        self.row_count = 0

    def pool_index(self, value: str) -> int:
        index = self.pool_indexes.get(value)
        if index is None:
            index = len(self.pool_values)
            self.pool_indexes[value] = index
            self.pool_values.append(value)
        return index

    def add_row(
        self,
        *,
        encoding: str,
        asm: str,
        inputs: list[tuple[str, str]],
        outputs: list[tuple[str, str]],
        exception: str | None,
    ) -> None:
        input_schema = tuple(key for key, _ in inputs)
        input_indexes = ",".join(str(self.pool_index(value)) for _, value in inputs)

        if exception:
            output_schema: tuple[str, ...] = ()
            output_text = "!" + exception
        else:
            output_schema = tuple(key for key, _ in outputs)
            output_text = ",".join(str(self.pool_index(value)) for _, value in outputs)

        group_key = (encoding, asm, input_schema, output_schema)
        group = self.groups.get(group_key)
        if group is None:
            group = Group(encoding, asm, input_schema, output_schema)
            self.groups[group_key] = group
        group.rows.append(input_indexes + "|" + output_text)
        self.row_count += 1

    def write(self) -> None:
        with self.path.open("w", encoding="utf-8", newline="\n") as out:
            out.write(f"data:{len(self.pool_values)}\n")
            for value in self.pool_values:
                out.write(f"#{value}\n")
            for group in self.groups.values():
                in_schema = ",".join(group.input_schema)
                out_schema = ",".join(group.output_schema)
                out.write(
                    f"instr:0x{self.address:X};#{group.encoding};{group.asm};"
                    f"{len(group.rows)};in={in_schema};out={out_schema}\n"
                )
                for row in group.rows:
                    out.write(row + "\n")


SAFE_FILENAME_RE = re.compile(r"[^A-Za-z0-9_.+-]+")


def parse_int(text: str) -> int:
    return int(text, 0)


def csv_values(text: str | None) -> list[str]:
    if not text:
        return []
    return [part.strip().lower() for part in text.split(",") if part.strip()]


def quote_sql_string(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def normalize_state_key(key: str) -> str:
    lowered = key.strip().lower()
    if lowered in {"flag", "flags", "eflags", "rflags"}:
        return "flags"
    return lowered


def normalize_hex(value: object, *, context: str) -> str:
    if not isinstance(value, str):
        raise ConversionError(f"{context} value is not a hex string: {value!r}")
    clean = "".join(value.split())
    if clean.startswith(("0x", "0X")):
        clean = clean[2:]
    if len(clean) % 2:
        raise ConversionError(f"{context} hex string has odd length: {value!r}")
    try:
        bytes.fromhex(clean)
    except ValueError as exc:
        raise ConversionError(f"{context} has invalid hex {value!r}: {exc}") from exc
    return clean.upper()


def normalize_encoding(value: object) -> str:
    if not isinstance(value, str):
        raise ConversionError(f"encoding is not a hex string: {value!r}")
    encoding = normalize_hex(value, context="encoding")
    if not encoding:
        raise ConversionError("encoding is empty")
    return encoding


def parse_state_json(text: str | None, *, context: str) -> list[tuple[str, str]]:
    if text is None or text == "":
        return []
    try:
        state = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ConversionError(f"{context} is invalid JSON: {exc}") from exc
    if not isinstance(state, dict):
        raise ConversionError(f"{context} JSON is not an object: {text!r}")

    items: list[tuple[str, str]] = []
    seen: set[str] = set()
    for raw_key, raw_value in state.items():
        key = normalize_state_key(str(raw_key))
        if not key:
            raise ConversionError(f"{context} contains an empty state key")
        if key in seen:
            raise ConversionError(f"{context} contains duplicate state key {key!r}")
        seen.add(key)
        items.append((key, normalize_hex(raw_value, context=f"{context}.{key}")))
    return items


def normalize_exception(value: object) -> str | None:
    if value is None:
        return None
    exception = str(value).strip()
    if not exception or exception.upper() == "NULL":
        return None
    if any(ch.isspace() for ch in exception):
        raise ConversionError(f"exception contains whitespace: {exception!r}")
    return exception


def output_filename(mnemonic: object) -> str:
    if mnemonic is None:
        raise ConversionError("mnemonic is NULL")
    name = str(mnemonic).strip().lower()
    if not name:
        raise ConversionError("mnemonic is empty")
    name = SAFE_FILENAME_RE.sub("_", name)
    return name + ".txt"


def prepare_output_dir(path: Path, *, overwrite: bool) -> None:
    if path.exists() and not path.is_dir():
        raise ConversionError(f"output path exists and is not a directory: {path}")
    path.mkdir(parents=True, exist_ok=True)
    existing = sorted(path.glob("*.txt"))
    if existing and not overwrite:
        raise ConversionError(
            f"output directory already contains .txt files: {path}; use --overwrite"
        )
    if overwrite:
        for existing_file in existing:
            existing_file.unlink()


def discover_inputs(paths: list[Path], input_dirs: list[Path]) -> list[Path]:
    inputs: list[Path] = []
    for path in paths:
        if not path.is_file():
            raise ConversionError(f"input parquet file does not exist: {path}")
        inputs.append(path)

    for directory in input_dirs:
        data_dir = directory / "data" if (directory / "data").is_dir() else directory
        if not data_dir.is_dir():
            raise ConversionError(f"input directory does not exist: {directory}")
        inputs.extend(sorted(data_dir.glob("*.parquet")))

    unique: OrderedDict[Path, None] = OrderedDict()
    for path in inputs:
        unique[path.resolve()] = None
    result = list(unique.keys())
    if not result:
        raise ConversionError("no input parquet files selected")
    return result


def validate_columns(con: duckdb.DuckDBPyConnection, parquet_path: Path) -> None:
    path_sql = quote_sql_string(parquet_path.as_posix())
    rows = con.execute(f"DESCRIBE SELECT * FROM read_parquet({path_sql})").fetchall()
    columns = {row[0] for row in rows}
    required = {"mnemonic", "asm", "encoding", "inputs", "outputs", "exception"}
    missing = sorted(required - columns)
    if missing:
        raise ConversionError(
            f"{parquet_path} is missing required columns: {', '.join(missing)}"
        )


def convert_one(
    parquet_path: Path,
    output_dir: Path,
    *,
    mnemonics: list[str],
    limit_rows: int | None,
    address: int,
    overwrite: bool,
) -> tuple[int, int, int]:
    prepare_output_dir(output_dir, overwrite=overwrite)

    con = duckdb.connect(database=":memory:")
    validate_columns(con, parquet_path)
    path_sql = quote_sql_string(parquet_path.as_posix())

    where = ""
    params: list[object] = []
    if mnemonics:
        placeholders = ",".join("?" for _ in mnemonics)
        where = f" WHERE lower(mnemonic) IN ({placeholders})"
        params.extend(mnemonics)

    query = (
        "SELECT mnemonic, asm, encoding, inputs, outputs, exception "
        f"FROM read_parquet({path_sql})"
        f"{where} "
        "ORDER BY lower(mnemonic), encoding, asm"
    )
    if limit_rows is not None:
        query += " LIMIT ?"
        params.append(limit_rows)

    cursor = con.execute(query, params)
    current_file: str | None = None
    corpus: OutputCorpus | None = None
    output_files = 0
    total_groups = 0
    total_rows = 0

    def flush() -> None:
        nonlocal corpus, output_files, total_groups, total_rows
        if corpus is None:
            return
        corpus.write()
        output_files += 1
        total_groups += len(corpus.groups)
        total_rows += corpus.row_count
        corpus = None

    while True:
        rows = cursor.fetchmany(FETCH_CHUNK_SIZE)
        if not rows:
            break
        for mnemonic, asm, encoding, inputs_json, outputs_json, exception_value in rows:
            filename = output_filename(mnemonic)
            if current_file != filename:
                flush()
                current_file = filename
                corpus = OutputCorpus(output_dir / filename, address=address)

            assert corpus is not None
            exception = normalize_exception(exception_value)
            corpus.add_row(
                encoding=normalize_encoding(encoding),
                asm=str(asm).strip(),
                inputs=parse_state_json(inputs_json, context="inputs"),
                outputs=([] if exception else parse_state_json(outputs_json, context="outputs")),
                exception=exception,
            )

    flush()
    return output_files, total_groups, total_rows


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convert BinPrey/x86-instruction-test-vectors parquet files to "
            "remill-tester pooled text files."
        )
    )
    parser.add_argument(
        "--input",
        dest="inputs",
        type=Path,
        action="append",
        default=[],
        help="Input CPU parquet file. May be repeated.",
    )
    parser.add_argument(
        "--input-dir",
        dest="input_dirs",
        type=Path,
        action="append",
        default=[],
        help=(
            "Directory containing CPU parquet files, or a cloned "
            "x86-instruction-test-vectors repository with a data/ directory."
        ),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        required=True,
        help=(
            "Output corpus directory. With one input parquet, mnemonic .txt files "
            "are written directly here. With multiple inputs, one subdirectory per "
            "parquet stem is created here."
        ),
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Remove existing .txt files in output corpus directories first.",
    )
    parser.add_argument(
        "--mnemonic",
        help="Comma-separated mnemonic filter, e.g. adc,add,xor.",
    )
    parser.add_argument(
        "--limit-rows",
        type=int,
        help="Maximum rows to convert per input parquet after filtering.",
    )
    parser.add_argument(
        "--address",
        type=parse_int,
        default=DEFAULT_ADDRESS,
        help=f"Instruction address to emit in headers (default 0x{DEFAULT_ADDRESS:X}).",
    )
    args = parser.parse_args(list(argv))
    if args.limit_rows is not None and args.limit_rows <= 0:
        parser.error("--limit-rows must be positive")
    if args.address < 0:
        parser.error("--address must be non-negative")
    if not args.inputs and not args.input_dirs:
        parser.error("provide --input or --input-dir")
    return args


def main(argv: Iterable[str] = sys.argv[1:]) -> int:
    args = parse_args(argv)
    try:
        inputs = discover_inputs(args.inputs, args.input_dirs)
        mnemonics = csv_values(args.mnemonic)
        multiple_inputs = len(inputs) > 1
        grand_files = 0
        grand_groups = 0
        grand_rows = 0
        for parquet_path in inputs:
            output_dir = args.output_dir / parquet_path.stem if multiple_inputs else args.output_dir
            files, groups, rows = convert_one(
                parquet_path,
                output_dir,
                mnemonics=mnemonics,
                limit_rows=args.limit_rows,
                address=args.address,
                overwrite=args.overwrite,
            )
            grand_files += files
            grand_groups += groups
            grand_rows += rows
            print(
                f"converted {parquet_path} -> {output_dir} "
                f"files={files} instruction_groups={groups} rows={rows}",
                file=sys.stderr,
            )
        print(
            f"converted_total inputs={len(inputs)} files={grand_files} "
            f"instruction_groups={grand_groups} rows={grand_rows}",
            file=sys.stderr,
        )
    except (ConversionError, duckdb.Error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
