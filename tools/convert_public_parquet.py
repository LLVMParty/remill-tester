#!/usr/bin/env python3
# /// script
# dependencies = [
#   "duckdb>=1.5.3,<2",
# ]
# ///
"""Convert the public binit parquet corpus to remill-tester text files.

The emitted format is the current x86Tester-style pooled text format:

    data:<N>
    #<hex 0>
    #<hex 1>
    ...
    instr:<address>;#<opcode_hex>;<disassembly>;<row_count>;in=<schema>;out=<schema>
    <input-indexes>|<output-indexes-or-!exception>

The public parquet corpus is already normalized to sparse integer state. Its
synthetic memory cell, ``mem0_value``, is converted into remill-tester's explicit
memory key syntax at ``mem[0x666666010100]``.
"""
from __future__ import annotations

import argparse
import json
import sys
import urllib.request
from pathlib import Path
from typing import Iterable

import duckdb


DEFAULT_BASE_URL = "https://binit.wazab.in"
FILES = ("instructions.parquet", "test_cases.parquet", "test_results.parquet")
ENTRY_ADDR = 0x666666660000
PAGE_SIZE = 0x1000
MEM0_ADDR = 0x666666010100
MASK64 = (1 << 64) - 1
FETCH_CHUNK_SIZE = 8192


class ConversionError(RuntimeError):
    pass


def parse_int(text: str) -> int:
    return int(text, 0)


def csv_values(text: str | None) -> list[str]:
    if not text:
        return []
    return [part.strip() for part in text.split(",") if part.strip()]


def quote_sql_string(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def ensure_data(base_url: str, data_dir: Path, *, download: bool) -> None:
    data_dir.mkdir(parents=True, exist_ok=True)
    missing = [name for name in FILES if not (data_dir / name).is_file()]
    if missing and not download:
        raise ConversionError(
            "missing parquet files in "
            f"{data_dir}: {', '.join(missing)}; rerun without --no-download"
        )

    for name in missing:
        url = f"{base_url.rstrip('/')}/data/{name}"
        destination = data_dir / name
        print(f"download {url} -> {destination}", file=sys.stderr)
        urllib.request.urlretrieve(url, destination)


def open_db(data_dir: Path) -> duckdb.DuckDBPyConnection:
    con = duckdb.connect(database=":memory:")
    for table in ("instructions", "test_cases", "test_results"):
        parquet = quote_sql_string((data_dir / f"{table}.parquet").as_posix())
        con.execute(f"CREATE VIEW {table} AS SELECT * FROM read_parquet({parquet})")
    return con


def validate_inputs(con: duckdb.DuckDBPyConnection) -> None:
    checks = {
        "initial_state_count does not match initial_states JSON length": """
            SELECT count(*)
            FROM test_cases
            WHERE json_array_length(initial_states::JSON) != initial_state_count
        """,
        "test_results references missing test_case_id": """
            SELECT count(*)
            FROM test_results tr
            LEFT JOIN test_cases tc USING (test_case_id)
            WHERE tc.test_case_id IS NULL
        """,
        "test_results metadata disagrees with test_cases": """
            SELECT count(*)
            FROM test_results tr
            JOIN test_cases tc USING (test_case_id)
            WHERE tr.instruction_id != tc.instruction_id
               OR tr.instruction_name != tc.instruction_name
               OR tr.opcode != tc.opcode
        """,
        "test_results has NULL final_state without exception_kind": """
            SELECT count(*)
            FROM test_results
            WHERE final_state IS NULL AND exception_kind IS NULL
        """,
        "test_results has both final_state and exception_kind": """
            SELECT count(*)
            FROM test_results
            WHERE final_state IS NOT NULL AND exception_kind IS NOT NULL
        """,
    }

    errors = []
    for description, query in checks.items():
        count = con.execute(query).fetchone()[0]
        if count:
            errors.append(f"{description}: {count}")
    if errors:
        raise ConversionError("input validation failed:\n  " + "\n  ".join(errors))


def int_le_hex(value: int, size: int) -> str:
    mask = (1 << (size * 8)) - 1
    return (int(value) & mask).to_bytes(size, byteorder="little").hex().upper()


def uint64_le_hex(value: int) -> str:
    return int_le_hex(value, 8)


def normalize_state_key(key: str) -> str:
    lowered = key.strip().lower()
    if lowered in {"flag", "flags", "eflags", "rflags"}:
        return "flags"
    return lowered


def state_value_hex(key: str, value: int) -> str:
    if key == "flags":
        return int_le_hex(value, 4)
    return uint64_le_hex(value)


def state_items(
    state_json: str | None,
    *,
    mem0_addr: int,
    extra_memory: list[tuple[int, bytes]] | None = None,
) -> list[tuple[str, str]]:
    if not state_json:
        state = {}
    else:
        state = json.loads(state_json)
        if not isinstance(state, dict):
            raise ConversionError(f"state JSON is not an object: {state_json!r}")

    parts: list[tuple[str, str]] = []
    for raw_key, raw_value in state.items():
        value = int(raw_value)
        key = normalize_state_key(raw_key)
        if key == "mem0_value":
            parts.append((f"mem[0x{mem0_addr:X}]", uint64_le_hex(value)))
        else:
            parts.append((key, state_value_hex(key, value)))
    for address, data in extra_memory or []:
        parts.append((f"mem[0x{address:X}]", data.hex().upper()))
    return parts


def instruction_address(opcode: str, *, entry_addr: int, page_size: int) -> int:
    clean_opcode = "".join(opcode.split())
    if len(clean_opcode) % 2:
        raise ConversionError(f"opcode has odd hex length: {opcode!r}")
    try:
        byte_length = len(bytes.fromhex(clean_opcode))
    except ValueError as exc:
        raise ConversionError(f"invalid opcode hex {opcode!r}: {exc}") from exc
    return entry_addr + page_size - byte_length


def sign_extend(value: int, bits: int) -> int:
    value &= (1 << bits) - 1
    sign_bit = 1 << (bits - 1)
    return value - (1 << bits) if value & sign_bit else value


def bit_test_memory_width(instruction: str) -> int | None:
    text = instruction.lower()
    if "qword ptr [" in text:
        return 8
    if "dword ptr [" in text:
        return 4
    if "word ptr [" in text:
        return 2
    return None


def bit_test_register_width(source: str) -> int | None:
    source = source.strip().lower()
    return {"rbx": 64, "ebx": 32, "bx": 16}.get(source)


def public_bit_test_hidden_memory(
    instruction_name: str,
    instruction: str,
    initial_state_json: str | None,
    final_state_json: str | None,
    *,
    mem0_addr: int,
) -> list[tuple[int, bytes]]:
    """Materialize public-corpus bit-test memory hidden behind mem0_value.

    The parquet schema serializes only the 8-byte mem0 cell. Register-offset
    memory forms of BT/BTC/BTR/BTS can legally access neighboring words/dwords/
    qwords in the same bit string, and the public results contain CF values from
    those neighboring cells. Derive the one input bit needed by the row's CF
    oracle so each converted text row remains independently executable.
    """
    if instruction_name.upper() not in {"BT", "BTC", "BTR", "BTS"}:
        return []
    element_size = bit_test_memory_width(instruction)
    if element_size is None:
        return []
    if initial_state_json is None or final_state_json is None:
        return []

    operands = instruction.rsplit(",", 1)
    if len(operands) != 2:
        return []
    register_bits = bit_test_register_width(operands[1])
    if register_bits is None:
        return []

    initial_state = json.loads(initial_state_json)
    final_state = json.loads(final_state_json)
    if "rbx" not in initial_state or "flag" not in final_state:
        return []

    element_bits = element_size * 8
    bit_offset = sign_extend(int(initial_state["rbx"]), register_bits)
    element_index = bit_offset // element_bits
    bit_index = bit_offset % element_bits
    element_addr = mem0_addr + element_index * element_size

    if mem0_addr <= element_addr and element_addr + element_size <= mem0_addr + 8:
        return []
    if (int(final_state["flag"]) & 1) == 0:
        return []

    value = 1 << bit_index
    return [(element_addr, bytes.fromhex(int_le_hex(value, element_size)))]


class OutputFile:
    def __init__(self, path: Path):
        self.path = path
        self.data_pool: list[str] = []
        self.data_index: dict[str, int] = {}
        self.body: list[str] = []

    def intern(self, hex_value: str) -> int:
        clean = "".join(str(hex_value).split()).upper()
        if len(clean) % 2:
            raise ConversionError(f"pooled value has odd hex length: {hex_value!r}")
        try:
            bytes.fromhex(clean)
        except ValueError as exc:
            raise ConversionError(f"invalid pooled hex value {hex_value!r}: {exc}") from exc
        existing = self.data_index.get(clean)
        if existing is not None:
            return existing
        index = len(self.data_pool)
        self.data_index[clean] = index
        self.data_pool.append(clean)
        return index

    def write(self, text: str) -> None:
        self.body.append(text)

    def flush(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.path.open("w", encoding="utf-8", newline="\n") as handle:
            handle.write(f"data:{len(self.data_pool)}\n")
            for hex_value in self.data_pool:
                handle.write(f"#{hex_value}\n")
            handle.writelines(self.body)


class OutputManager:
    def __init__(self, output: Path | None, output_dir: Path | None, *, overwrite: bool):
        if (output is None) == (output_dir is None):
            raise ConversionError("provide exactly one of --output or --output-dir")
        self.output = output
        self.output_dir = output_dir
        self.overwrite = overwrite
        self._single_file: OutputFile | None = None
        self._files: dict[str, OutputFile] = {}
        self._paths: list[Path] = []

    @property
    def paths(self) -> list[Path]:
        return self._paths

    def __enter__(self) -> "OutputManager":
        if self.output is not None:
            if self.output.exists() and not self.overwrite:
                raise ConversionError(f"output already exists: {self.output}")
            self._single_file = OutputFile(self.output)
            self._paths.append(self.output)
        else:
            assert self.output_dir is not None
            self.output_dir.mkdir(parents=True, exist_ok=True)
            if not self.overwrite and any(self.output_dir.glob("*.txt")):
                raise ConversionError(
                    f"output directory already contains .txt files: {self.output_dir}"
                )
            if self.overwrite:
                for old in self.output_dir.glob("*.txt"):
                    old.unlink()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        if exc_type is not None:
            return
        if self._single_file is not None:
            self._single_file.flush()
        for output_file in self._files.values():
            output_file.flush()

    def handle_for(self, instruction_name: str) -> OutputFile:
        if self._single_file is not None:
            return self._single_file
        safe_name = instruction_name.lower().replace("/", "_")
        output_file = self._files.get(safe_name)
        if output_file is None:
            assert self.output_dir is not None
            path = self.output_dir / f"{safe_name}.txt"
            output_file = OutputFile(path)
            self._files[safe_name] = output_file
            self._paths.append(path)
        return output_file


def build_query(*, mnemonics: list[str], limit_test_cases: int | None) -> tuple[str, list[object]]:
    filters = []
    params: list[object] = []
    if mnemonics:
        placeholders = ", ".join("?" for _ in mnemonics)
        filters.append(f"lower(tc.instruction_name) IN ({placeholders})")
        params.extend(m.lower() for m in mnemonics)
    where = "WHERE " + " AND ".join(filters) if filters else ""

    case_limit = ""
    if limit_test_cases is not None:
        case_limit = "LIMIT ?"
        params.append(limit_test_cases)

    # The limited_cases CTE keeps --limit-test-cases applying to whole test
    # cases. State limiting, if requested, is applied by the Python writer so a
    # partially-emitted test case still gets an accurate header row count.
    query = f"""
        WITH limited_cases AS (
            SELECT tc.*
            FROM test_cases tc
            {where}
            ORDER BY tc.instruction_name, tc.test_case_id
            {case_limit}
        )
        SELECT
            tc.test_case_id,
            tc.instruction,
            tc.opcode,
            tc.instruction_id,
            tc.instruction_name,
            tc.initial_state_count,
            tr.state_index,
            json_extract(tc.initial_states::JSON, tr.state_index) AS initial_state,
            tr.final_state,
            tr.exception_kind
        FROM limited_cases tc
        JOIN test_results tr USING (test_case_id)
        ORDER BY tc.instruction_name, tc.test_case_id, tr.state_index
    """
    return query, params


def append_schema(schema: list[str], seen: set[str], items: list[tuple[str, str]]) -> None:
    for key, _value in items:
        if key not in seen:
            seen.add(key)
            schema.append(key)


def items_to_map(items: list[tuple[str, str]], *, context: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for key, value in items:
        if key in result:
            raise ConversionError(f"duplicate key {key!r} in {context}")
        result[key] = value
    return result


def is_memory_key(key: str) -> bool:
    lowered = key.lower()
    return (lowered.startswith("mem[") and lowered.endswith("]")) or lowered.startswith("mem@")


def schema_indices(
    output_file: OutputFile,
    values: dict[str, str],
    schema: list[str],
    *,
    context: str,
    defaults: dict[str, str] | None = None,
) -> str:
    indices: list[str] = []
    for key in schema:
        value = values.get(key)
        if value is None and defaults is not None:
            value = defaults.get(key)
        if value is None:
            raise ConversionError(f"missing key {key!r} in {context}")
        indices.append(str(output_file.intern(value)))
    return ",".join(indices)


def write_group(
    outputs: OutputManager,
    group: list[tuple],
    *,
    entry_addr: int,
    page_size: int,
    mem0_addr: int,
) -> int:
    if not group:
        return 0

    (
        test_case_id,
        instruction,
        opcode,
        _instruction_id,
        instruction_name,
        _initial_state_count,
        _state_index,
        _initial_state,
        _final_state,
        _exception_kind,
    ) = group[0]
    opcode = str(opcode).upper()
    address = instruction_address(opcode, entry_addr=entry_addr, page_size=page_size)
    output_file = outputs.handle_for(str(instruction_name))

    in_schema: list[str] = []
    out_schema: list[str] = []
    in_seen: set[str] = set()
    out_seen: set[str] = set()
    input_defaults: dict[str, str] = {}
    converted_rows: list[tuple[dict[str, str], dict[str, str], str | None]] = []

    for row_index, row in enumerate(group):
        initial_state = row[7]
        final_state = row[8]
        exception_kind = row[9]
        exception_text = str(exception_kind) if exception_kind is not None else None
        extra_initial_memory = public_bit_test_hidden_memory(
            str(instruction_name),
            str(instruction),
            initial_state,
            final_state,
            mem0_addr=mem0_addr,
        )
        input_items = state_items(
            initial_state,
            mem0_addr=mem0_addr,
            extra_memory=extra_initial_memory,
        )
        output_items = [] if exception_text else state_items(final_state, mem0_addr=mem0_addr)
        append_schema(in_schema, in_seen, input_items)
        for key, value in input_items:
            if is_memory_key(key) and key not in input_defaults:
                input_defaults[key] = "00" * (len(value) // 2)
        if not exception_text:
            append_schema(out_schema, out_seen, output_items)
        converted_rows.append(
            (
                items_to_map(
                    input_items,
                    context=f"test_case {test_case_id} row {row_index} input",
                ),
                items_to_map(
                    output_items,
                    context=f"test_case {test_case_id} row {row_index} output",
                ),
                exception_text,
            )
        )

    output_file.write(
        f"instr:0x{address:X};#{opcode};{instruction};{len(converted_rows)};"
        f"in={','.join(in_schema)};out={','.join(out_schema)}\n"
    )

    for row_index, (input_values, output_values, exception_text) in enumerate(converted_rows):
        input_text = schema_indices(
            output_file,
            input_values,
            in_schema,
            context=f"test_case {test_case_id} row {row_index} input",
            defaults=input_defaults,
        )
        if exception_text:
            output_text = f"!{exception_text}"
        else:
            output_text = schema_indices(
                output_file,
                output_values,
                out_schema,
                context=f"test_case {test_case_id} row {row_index} output",
            )
        output_file.write(f"{input_text}|{output_text}\n")
    return len(converted_rows)


def convert(
    con: duckdb.DuckDBPyConnection,
    outputs: OutputManager,
    *,
    mnemonics: list[str],
    limit_test_cases: int | None,
    limit_states: int | None,
    entry_addr: int,
    page_size: int,
    mem0_addr: int,
) -> tuple[int, int]:
    query, params = build_query(mnemonics=mnemonics, limit_test_cases=limit_test_cases)
    cursor = con.execute(query, params)

    current_case_id: int | None = None
    group: list[tuple] = []
    emitted_cases = 0
    emitted_states = 0
    stop = False

    while not stop:
        rows = cursor.fetchmany(FETCH_CHUNK_SIZE)
        if not rows:
            break
        for row in rows:
            test_case_id = int(row[0])
            if current_case_id is None:
                current_case_id = test_case_id
            if test_case_id != current_case_id:
                written = write_group(
                    outputs,
                    group,
                    entry_addr=entry_addr,
                    page_size=page_size,
                    mem0_addr=mem0_addr,
                )
                emitted_states += written
                emitted_cases += 1
                group = []
                current_case_id = test_case_id
                if limit_states is not None and emitted_states >= limit_states:
                    stop = True
                    break

            if limit_states is not None:
                remaining = limit_states - emitted_states - len(group)
                if remaining <= 0:
                    stop = True
                    break
            group.append(row)

    if group and (limit_states is None or emitted_states < limit_states):
        if limit_states is not None:
            group = group[: max(0, limit_states - emitted_states)]
        if group:
            written = write_group(
                outputs,
                group,
                entry_addr=entry_addr,
                page_size=page_size,
                mem0_addr=mem0_addr,
            )
            emitted_states += written
            emitted_cases += 1

    return emitted_cases, emitted_states


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert public binit parquet files to remill-tester text format."
    )
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL)
    parser.add_argument(
        "--data-dir",
        type=Path,
        default=Path(".cache/binit-parquet"),
        help="Directory containing/downloading the three parquet files.",
    )
    parser.add_argument(
        "--no-download",
        action="store_true",
        help="Require parquet files to already exist in --data-dir.",
    )
    output_group = parser.add_mutually_exclusive_group(required=True)
    output_group.add_argument("--output", type=Path, help="Write one combined .txt file.")
    output_group.add_argument(
        "--output-dir",
        type=Path,
        help="Write one <instruction_name>.txt file per instruction name.",
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Overwrite --output or remove existing .txt files in --output-dir.",
    )
    parser.add_argument(
        "--mnemonic",
        help="Comma-separated instruction-name filter, e.g. adc,add,xor.",
    )
    parser.add_argument("--limit-test-cases", type=int)
    parser.add_argument("--limit-states", type=int)
    parser.add_argument(
        "--entry-addr",
        type=parse_int,
        default=ENTRY_ADDR,
        help=f"Synthetic code page base used by the parquet oracle (default 0x{ENTRY_ADDR:X}).",
    )
    parser.add_argument(
        "--page-size",
        type=parse_int,
        default=PAGE_SIZE,
        help=f"Synthetic code page size (default 0x{PAGE_SIZE:X}).",
    )
    parser.add_argument(
        "--mem0-addr",
        type=parse_int,
        default=MEM0_ADDR,
        help=f"Address for mem0_value (default 0x{MEM0_ADDR:X}).",
    )
    parser.add_argument(
        "--skip-validation",
        action="store_true",
        help="Skip parquet referential-integrity checks.",
    )
    args = parser.parse_args(list(argv))
    if args.limit_test_cases is not None and args.limit_test_cases <= 0:
        parser.error("--limit-test-cases must be positive")
    if args.limit_states is not None and args.limit_states <= 0:
        parser.error("--limit-states must be positive")
    return args


def main(argv: Iterable[str] = sys.argv[1:]) -> int:
    args = parse_args(argv)
    try:
        ensure_data(args.base_url, args.data_dir, download=not args.no_download)
        con = open_db(args.data_dir)
        if not args.skip_validation:
            validate_inputs(con)
        mnemonics = csv_values(args.mnemonic)
        with OutputManager(args.output, args.output_dir, overwrite=args.overwrite) as outputs:
            cases, states = convert(
                con,
                outputs,
                mnemonics=mnemonics,
                limit_test_cases=args.limit_test_cases,
                limit_states=args.limit_states,
                entry_addr=args.entry_addr,
                page_size=args.page_size,
                mem0_addr=args.mem0_addr,
            )
            output_paths = list(outputs.paths)
        print(
            f"converted test_cases={cases} states={states} files={len(output_paths)}",
            file=sys.stderr,
        )
        for path in output_paths[:10]:
            print(f"wrote {path}", file=sys.stderr)
        if len(output_paths) > 10:
            print(f"... {len(output_paths) - 10} more files", file=sys.stderr)
    except ConversionError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
