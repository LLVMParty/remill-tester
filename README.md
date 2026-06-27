# remill-tester

`remill-tester` is a Remill instruction-semantics test harness for x86-64. It takes hardware-generated instruction test vectors, decodes each instruction with XED, lifts it with Remill, JIT-executes the lifted instruction, and compares the resulting sparse state against the hardware oracle.

The point is to make Remill semantic regressions visible while keeping unsupported coverage gaps separate from real mismatches. The tester handles scalar/vector registers, user-visible RFLAGS masking using XED undefined-flag metadata, normalized memory cells, expected exception paths, and selected system/x87 cases. System instructions such as `VERR`/`VERW` remain modeled through Remill sync hypercalls; the harness supplies deterministic hypercall results for corpus execution.

## Repository layout

- `src/` — parser, XED metadata, Remill LLJIT backend, state bridge, memory model, and comparator.
- `tools/convert_test_vectors.py` — converter for the multi-CPU [`x86-instruction-test-vectors`](https://huggingface.co/datasets/BinPrey/x86-instruction-test-vectors) parquet repository.
- `tools/convert_public_parquet.py` — converter for the public [binit](https://binit.wazab.in/) parquet database.
- `tests/smoke_3975wx.txt` — small curated pooled-format smoke corpus for CI.
- `testdata/` — ignored local converted/generated corpus directories.
- `dependencies/remill/` — Remill submodule under test.
- `TESTED.md`, `failures.md`, `PLAN.md` — historical run notes and design notes.

## Build

Initialize submodules and build the dependency prefix:

```bash
git submodule update --init --recursive
cmake -G Ninja -S dependencies -B dependencies/build \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build dependencies/build
```

Build the tester and run the fast smoke tests:

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build
ctest --test-dir build --output-on-failure --timeout 30
```

For large sweeps, also build a Release binary:

```bash
cmake -G Ninja -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target remill-tester -j4
./build-release/remill-tester --self-test
```

## Getting test data

`remill-tester` consumes the current pooled [x86Tester](https://github.com/BinPrey/x86Tester) text format. The repository does not commit large corpora; convert or generate them into ignored `testdata/` directories. The converter scripts use DuckDB via Python dependency metadata; `uv run ...` will create the needed tool environment automatically.

### Option A: multi-CPU test-vector repository

This is the easiest way to run against hardware results from many users/CPUs.

```bash
git clone https://huggingface.co/datasets/BinPrey/x86-instruction-test-vectors \
  ../x86-instruction-test-vectors
# If your clone leaves Git LFS pointer files instead of parquet files:
# git -C ../x86-instruction-test-vectors lfs pull
ls ../x86-instruction-test-vectors/data/*.parquet
```

Convert one CPU parquet into one flat corpus directory:

```bash
uv run tools/convert_test_vectors.py \
  --input ../x86-instruction-test-vectors/data/<cpu>.parquet \
  --output-dir testdata/<cpu> \
  --overwrite
```

Convert only a small subset while iterating:

```bash
uv run tools/convert_test_vectors.py \
  --input ../x86-instruction-test-vectors/data/<cpu>.parquet \
  --output-dir testdata/<cpu>-adc-add \
  --mnemonic adc,add \
  --limit-rows 10000 \
  --overwrite
```

Convert every CPU parquet. With multiple inputs, the converter creates one subdirectory per parquet stem:

```bash
uv run tools/convert_test_vectors.py \
  --input-dir ../x86-instruction-test-vectors \
  --output-dir testdata/test-vectors \
  --overwrite
```

Run each resulting CPU directory separately, e.g. `testdata/test-vectors/13th_Gen_Intel_R_Core_TM_i9_13900HK_f6m186s2`.

### Option B: public binit parquet database

This converter downloads the three public parquet tables into `.cache/binit-parquet` unless `--no-download` is supplied:

```bash
uv run tools/convert_public_parquet.py \
  --output-dir testdata/public-binit \
  --overwrite
```

Useful subset options are the same idea:

```bash
uv run tools/convert_public_parquet.py \
  --output-dir testdata/public-binit-adc-add \
  --mnemonic adc,add \
  --limit-states 10000 \
  --overwrite
```

### Option C: generated x86Tester data

If you generate data directly with a current x86Tester build, place the emitted pooled `.txt` files under `testdata/<corpus>/`. No conversion is needed.

## Running tests

Parse/decode a corpus without executing lifted code:

```bash
/usr/bin/time -p ./build-release/remill-tester --input-dir testdata/<corpus>
```

Execute one file and stop at the first mismatch:

```bash
./build-release/remill-tester testdata/<corpus>/xor.txt \
  --execute --stop-on-first-fail
```

Execute part of a corpus:

```bash
./build-release/remill-tester --input-dir testdata/<corpus> \
  --mnemonic adc,add,xor \
  --limit-states 50000 \
  --execute \
  --stop-on-first-fail
```

Run a full corpus and save mismatch/skip reports:

```bash
/usr/bin/time -p ./build-release/remill-tester --input-dir testdata/<corpus> \
  --execute \
  --report-jsonl .cache/mismatches.jsonl \
  --report-skips-jsonl .cache/skips.jsonl
```

`--execute` exits non-zero when any selected, non-skipped row mismatches. Skips are reported separately with reasons such as unsupported XED decode, unsupported Remill lift, unsupported state, or intentionally unsupported FPU cases.

## Corpus format

The parser expects the current pooled x86Tester text format only; the older inline row format is not supported.

```text
data:<pool-count>
#<hex bytes>
#<hex bytes>
...
instr:<address>;#<opcode_hex>;<assembly>;<row_count>;in=<schema>;out=<schema>
<input-pool-indexes>|<output-pool-indexes>
<input-pool-indexes>|!<exception>
```

Schema entries are comma-separated state keys. Register keys are canonicalized by the tester, `flags`/`rflags`/`eflags` are treated as flags, and memory cells use `mem[0x...]`. Pooled values are raw hex bytes in the order emitted by x86Tester; scalar values are read little-endian.

## Interpreting results

A modern x86 corpus contains many instructions that Remill cannot currently decode or lift, plus floating-point/x87 cases that can differ by host behavior. Treat the summary, skip buckets, and JSONL mismatch reports as the source of truth for a given run rather than assuming that every x86 instruction is supported or semantically identical.
