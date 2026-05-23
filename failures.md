# Failure and Skip Triage

This file records currently known non-green areas separately from `TESTED.md`.

## Current semantic failures

Known current semantic mismatches in the committed tester state:

| Area | Example command | Current result | Notes |
|---|---|---|---|
| Approximate reciprocal square root | `./build-release/remill-tester 3975WX/rsqrtss.txt --execute --stop-on-first-fail` | First mismatch at state 65: expected `xmm0=0x5EB50000`, actual `0x5EB504F3` | Remill computes a precise reciprocal square root path; 3975WX returns the architectural approximate result. |

The earlier SSE FP-to-integer invalid-conversion issue is fixed by Remill submodule commit `3bafb8a`:

- Before the fix, `cvtpd2dq.txt` failed at `cvtpd2dq xmm15, xmm15`; hardware returned the x86 indefinite integer `0x80000000` for an invalid lane, while Remill returned `0`.
- After the fix, full Release conversion runs pass:
  - `cvtdq2pd.txt`: `51,024 passed, 0 failed, 0 skipped`
  - `cvtdq2ps.txt`: `62,512 passed, 0 failed, 0 skipped`
  - `cvtpd2dq.txt`: `48,360 passed, 0 failed, 0 skipped`
  - `cvtpd2ps.txt`: `42,888 passed, 0 failed, 0 skipped`
  - `cvtps2dq.txt`: `42,984 passed, 0 failed, 0 skipped`
  - `cvtps2pd.txt`: `47,928 passed, 0 failed, 0 skipped`
  - `cvtsd2si.txt`: `45,312 passed, 0 failed, 0 skipped`
  - `cvtsd2ss.txt`: `58,360 passed, 0 failed, 0 skipped`
  - `cvtsi2sd.txt`: `112,128 passed, 0 failed, 0 skipped`
  - `cvtsi2ss.txt`: `116,480 passed, 0 failed, 0 skipped`
  - `cvtss2sd.txt`: `53,552 passed, 0 failed, 0 skipped`
  - `cvtss2si.txt`: `34,048 passed, 0 failed, 0 skipped`
  - `cvttpd2dq.txt`: `48,144 passed, 0 failed, 0 skipped`
  - `cvttps2dq.txt`: `40,856 passed, 0 failed, 0 skipped`
  - `cvttsd2si.txt`: `45,568 passed, 0 failed, 0 skipped`
  - `cvttss2si.txt`: `32,512 passed, 0 failed, 0 skipped`

The earlier legacy SSE packed-compare predicate issue is fixed by Remill submodule commit `a08d0ac`:

- Before the fix, `cmppd.txt` failed at `cmppd xmm7, xmm0, 0x0F` because Remill interpreted the immediate as the AVX 5-bit `TRUE_UQ` predicate instead of legacy SSE `imm8[2:0] == 7` (`ORD_Q`).
- After the fix, full Release runs pass:
  - `cmppd.txt`: `523,512 passed, 0 failed, 0 skipped`
  - `cmpps.txt`: `497,792 passed, 0 failed, 0 skipped`
  - `comisd.txt`/`comiss.txt`/`ucomisd.txt`/`ucomiss.txt`: each `2,272 passed, 0 failed, 0 skipped`

The earlier `divpd`/`divps`/`divsd`/`divss` NaN-sign issue is fixed by Remill submodule commit `9c88816`:

- Before the fix, limited 20-row samples showed negative quiet-NaN mismatches: `divpd` 11 failures, `divps` 20 failures, `divsd` 9 failures, `divss` 12 failures.
- After the fix, full Release runs pass:
  - `divpd.txt`: `62,880 passed, 0 failed, 0 skipped`
  - `divps.txt`: `62,824 passed, 0 failed, 0 skipped`
  - `divsd.txt`: `61,944 passed, 0 failed, 0 skipped`
  - `divss.txt`: `59,912 passed, 0 failed, 0 skipped`

The earlier packed-double min/max and packed-single sqrt lift gaps are fixed by Remill submodule commit `1ddc9d1`:

- Before the fix, `maxpd.txt` skipped all `61,568` rows as `unsupported:remill_lift`.
- Before the fix, `sqrtps.txt` skipped all `64,168` rows as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `maxpd.txt`: `61,568 passed, 0 failed, 0 skipped`
  - `minpd.txt`: `61,768 passed, 0 failed, 0 skipped`
  - `sqrtps.txt`: `64,168 passed, 0 failed, 0 skipped`

The earlier `movmskpd` lift gap is fixed by Remill submodule commit `5a1b388`:

- Before the fix, a limited 20-row sample skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `movmskpd.txt`: `25,600 passed, 0 failed, 0 skipped`
  - `movmskps.txt`: `26,368 passed, 0 failed, 0 skipped`

The earlier `hsubpd`/`hsubps` lift gaps are fixed by Remill submodule commit `37d8e80`:

- Before the fix, limited 20-row samples skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `hsubpd.txt`: `63,992 passed, 0 failed, 0 skipped`
  - `hsubps.txt`: `63,408 passed, 0 failed, 0 skipped`
  - `haddpd.txt`: `62,816 passed, 0 failed, 0 skipped`
  - `haddps.txt`: `63,064 passed, 0 failed, 0 skipped`

The earlier `blendpd`/`blendps`/`blendvpd`/`blendvps` lift gaps are fixed by Remill submodule commit `7dfa6ba`:

- Before the fix, limited 20-row samples skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `blendpd.txt`: `529,968 passed, 0 failed, 0 skipped`
  - `blendps.txt`: `537,288 passed, 0 failed, 0 skipped`
  - `blendvpd.txt`: `61,346 passed, 0 failed, 0 skipped`
  - `blendvps.txt`: `60,230 passed, 0 failed, 0 skipped`

The earlier `extractps`/`insertps` lift gaps are fixed by Remill submodule commit `f00e89d`:

- Before the fix, limited 20-row samples skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `extractps.txt`: `124,160 passed, 0 failed, 0 skipped`
  - `insertps.txt`: `405,160 passed, 0 failed, 0 skipped`

The earlier `roundpd`/`roundps`/`roundsd`/`roundss` lift gaps are fixed by Remill submodule commit `ce2c8a5`:

- Before the fix, limited 20-row samples skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `roundpd.txt`: `454,512 passed, 0 failed, 0 skipped`
  - `roundps.txt`: `479,064 passed, 0 failed, 0 skipped`
  - `roundsd.txt`: `506,000 passed, 0 failed, 0 skipped`
  - `roundss.txt`: `513,920 passed, 0 failed, 0 skipped`

The earlier `dppd`/`dpps` lift gaps are fixed by Remill submodule commit `6d77129`:

- Before the fix, limited 20-row samples skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `dppd.txt`: `353,856 passed, 0 failed, 0 skipped`
  - `dpps.txt`: `350,928 passed, 0 failed, 0 skipped`

The earlier `mulx` destination-ordering mismatch is fixed by Remill submodule commit `d000b5c`:

- Before the fix, `mulx r13d, r13d, r8d` showed an aliasing mismatch (`r13` expected high half, actual low half).
- A first attempt exposed non-aliasing destination reversal on `mulx r14d, r13d, r12d`.
- After the fix, full Release `mulx.txt` passes: `1,520,573 passed, 0 failed, 0 skipped`.

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
| Remill lift unsupported: packed single reciprocal sqrt | `./build-release/remill-tester 3975WX/rsqrtps.txt --execute --limit-states 20 --stop-on-first-fail` | `20 skipped`; `unsupported:remill_lift=20` | Scalar `rsqrtss` lifts but currently mismatches the 3975WX approximate result. |
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
