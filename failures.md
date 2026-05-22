# Failure and Skip Triage

This file records currently known non-green areas separately from `TESTED.md`.

## Current semantic failures

No current full-file semantic mismatches are known in the committed tester state.

The earlier `movzx`/`movsx`/`movsxd` issue is fixed by Remill submodule commit `d83d754` and parent commit `160f119`:

- `./build-release/remill-tester 3975WX/movzx.txt --execute --stop-on-first-fail`
  - `75,022 passed, 0 failed, 0 skipped`
- `./build-release/remill-tester 3975WX/movsx.txt --execute --stop-on-first-fail`
  - `113,322 passed, 0 failed, 0 skipped`
- `./build-release/remill-tester 3975WX/movsxd.txt --execute --stop-on-first-fail`
  - `54,256 passed, 0 failed, 0 skipped`

The earlier `rol`/`ror` CF mismatches are also fixed in the comparator by dynamically ignoring flags for no-op rotate counts when the input row does not provide initial flags.

## Current skips / unsupported areas

These are not counted as semantic failures by the runner (`execution_failed=0`), but they are outstanding coverage gaps.

| Area | Example command | Current result | Notes |
|---|---|---|---|
| Raw memory-oracle gaps | `./build-release/remill-tester 3975WX/xlat.txt --execute --stop-on-first-fail` | `0 passed, 0 failed, 16 skipped`; `memory_state_missing=16` | Raw `3975WX` rows do not serialize table memory contents. Needs normalized `mem[...]` input/output cells or generator changes. |
| Remill lift unsupported: scalar BMI1 | `./build-release/remill-tester 3975WX/blsi.txt --execute --limit-states 20 --stop-on-first-fail` | `20 skipped`; `unsupported:remill_lift=20` | Also observed for `blsr` and `blsmsk`. |
| Remill lift unsupported: packed double min/max | `./build-release/remill-tester 3975WX/maxpd.txt --execute --limit-states 20 --stop-on-first-fail` | `20 skipped`; `unsupported:remill_lift=20` | `maxps`/`minps` full files pass; packed-double variants need Remill lift support/triage. |
| Remill lift unsupported: AES | `./build-release/remill-tester 3975WX/aesenc.txt --execute --limit-states 20 --stop-on-first-fail` | `20 skipped`; `unsupported:remill_lift=20` | Also observed for `aesdec`. |
| x87/FPU state unsupported | `./build-release/remill-tester 3975WX/fadd.txt --execute --limit-states 5 --stop-on-first-fail` | skipped as `fpu_state_unsupported` | Requires x87 state bridge and safe JIT support before comparing. |
| Privileged/IO/system instructions | e.g. `cli`, `rdtsc` limited runs | skipped as `privileged_or_io_unsupported` | Avoids host JIT fatal paths and nondeterministic system semantics. |

## Timeouts / long sweeps

A prior Release sweep over `lzcnt`, `tzcnt`, `bextr`, `rorx`, `sarx`, `shlx`, and `shrx` timed out at the shell level while entering `sarx`. That timeout is obsolete after Remill submodule commit `e1442e2`, which added `SARX`/`SHRX` semantics.

Current full-file Release results for that group are green:

- `lzcnt.txt`: `34,400 passed, 0 failed, 0 skipped`
- `tzcnt.txt`: `34,104 passed, 0 failed, 0 skipped`
- `bextr.txt`: `647,608 passed, 0 failed, 0 skipped`
- `rorx.txt`: `405,456 passed, 0 failed, 0 skipped`
- `shlx.txt`: `746,440 passed, 0 failed, 0 skipped`
- `sarx.txt`: `770,320 passed, 0 failed, 0 skipped`
- `shrx.txt`: `738,888 passed, 0 failed, 0 skipped`

## How to capture details

Use skip JSONL for unsupported rows:

```sh
./build-release/remill-tester 3975WX/blsi.txt \
  --execute --limit-states 20 \
  --report-skips-jsonl /tmp/blsi-skips.jsonl
```

The summary groups skips, while JSONL records preserve exact opcode/backend details.
