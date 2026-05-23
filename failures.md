# Failure and Skip Triage

This file records currently known non-green areas separately from `TESTED.md`.

## Current semantic failures

No current semantic mismatches are left in the committed tester state. Unsupported or unmodeled areas are tracked as skips below until the relevant state/oracle/semantics support exists.

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

The earlier `shld`/`shrd` 16-bit wide-count mismatch is fixed by Remill submodule commit `5180828`:

- Before the fix, `shld r9w, di, cl` with masked counts greater than 16 preserved the old destination and mismatched hardware results/flags.
- After the fix, full Release runs pass:
  - `shld.txt`: `585,136 passed, 0 failed, 0 skipped`
  - `shrd.txt`: `580,799 passed, 0 failed, 0 skipped`

The earlier `adcx`/`adox` lift gaps are fixed by Remill submodule commit `56f21a0`:

- Before the fix, limited 20-row samples skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `adcx.txt`: `49,216 passed, 0 failed, 0 skipped`
  - `adox.txt`: `49,552 passed, 0 failed, 0 skipped`

The earlier `mpsadbw` lift gap is fixed by Remill submodule commit `7db212d`:

- Before the fix, a limited 20-row sample skipped as `unsupported:remill_lift`.
- After the fix, full Release `mpsadbw.txt` passes: `470,232 passed, 0 failed, 0 skipped`.

The earlier `insertq` lift gap is fixed by Remill submodule commit `4277839`:

- Before the fix, `insertq.txt` had `unsupported:remill_lift` skips for immediate-control and register-control SSE4A forms.
- After the fix, full Release `insertq.txt` passes: `3,741,176 passed, 0 failed, 0 skipped`.

The earlier AES-NI lift gaps are fixed by Remill submodule commit `298519c`:

- Before the fix, limited 20-row samples for `aesenc`, `aesdec`, `aesenclast`, `aesdeclast`, `aesimc`, and `aeskeygenassist` skipped as `unsupported:remill_lift`.
- After the fix, full Release runs pass:
  - `aesenc.txt`: `64,664 passed, 0 failed, 0 skipped`
  - `aesenclast.txt`: `64,632 passed, 0 failed, 0 skipped`
  - `aesdec.txt`: `64,488 passed, 0 failed, 0 skipped`
  - `aesdeclast.txt`: `64,552 passed, 0 failed, 0 skipped`
  - `aesimc.txt`: `64,320 passed, 0 failed, 0 skipped`
  - `aeskeygenassist.txt`: `580,168 passed, 0 failed, 0 skipped`

The earlier `movzx`/`movsx`/`movsxd` issue is fixed by Remill submodule commit `d83d754` and parent commit `160f119`:

- `./build-release/remill-tester 3975WX/movzx.txt --execute --stop-on-first-fail`
  - `75,022 passed, 0 failed, 0 skipped`
- `./build-release/remill-tester 3975WX/movsx.txt --execute --stop-on-first-fail`
  - `113,322 passed, 0 failed, 0 skipped`
- `./build-release/remill-tester 3975WX/movsxd.txt --execute --stop-on-first-fail`
  - `54,256 passed, 0 failed, 0 skipped`

The earlier `rol`/`ror` CF mismatches are also fixed in the comparator by dynamically ignoring flags for no-op rotate counts when the input row does not provide initial flags.

The earlier approximate reciprocal/reciprocal-sqrt skips are fixed by Remill submodule commit `8f0c541` and the parent submodule update in this changeset:

- `./build-release/remill-tester 3975WX/rcpps.txt 3975WX/rcpss.txt 3975WX/rsqrtps.txt 3975WX/rsqrtss.txt --execute --stop-on-first-fail`
  - `229,872 passed, 0 failed, 0 skipped`

## Current skips / unsupported areas

These are not counted as semantic failures by the runner (`execution_failed=0`), but they are outstanding coverage gaps. A combined Release audit over the `89` files currently absent from `TESTED.md` selected `298,479` rows: `165,322 passed, 0 failed, 133,157 skipped`.

| Area | Example command | Current result | Notes |
|---|---|---|---|
| Raw memory-oracle gaps | combined remaining-file audit, including `xlat`, string ops, and `enter` | `23,118 skipped`; `memory_state_missing=23,118` | Raw `3975WX` rows do not serialize table/stack/string memory contents. Needs normalized `mem[...]` input/output cells or generator changes. |
| x87/FPU state unsupported | combined remaining-file audit over x87 files | `42,942 skipped`; `fpu_state_unsupported=42,942` | Requires the broader x87 stack/tag/control bridge before comparing arithmetic and data-transfer instructions. A minimal status-word bridge covers `fnstsw`, `fninit`, `fnclex`, `fincstp`, `fdecstp`, `ffree`, and `ffreep` (`583` rows passed). Remill submodule commits `6f6c240`/`04a8247`/`d49aaa5` and row-level filtering now also execute `fxam` plus canonical `fabs`/`fchs`/`fxch`/`fcmov*` rows (`645` additional rows passed) while skipping untrusted raw x87 stack encodings. |
| MMX state unsupported in raw corpus | combined audit over `movq2dq`, `cvtpi2pd`, and `cvtpi2ps` | `54,272 skipped`; `mmx_state_unsupported=54,272` | `/Users/admin/Projects/x86Tester/src/execution/execution.cpp` does not expose MMX registers in `getContextReg`, so raw rows with `mm0`..`mm7` do not provide a trustworthy hardware MMX oracle. |
| Privileged/IO instructions | e.g. `./build-release/remill-tester 3975WX/cli.txt 3975WX/lmsw.txt --execute --limit-states 1000` | skipped as `privileged_or_io_unsupported` | Avoids privileged host/JIT paths. |
| Environment reads | `./build-release/remill-tester 3975WX/rdtsc.txt 3975WX/rdtscp.txt 3975WX/rdpmc.txt 3975WX/rdpru.txt 3975WX/rdrand.txt 3975WX/rdseed.txt 3975WX/rdpid.txt 3975WX/rdgsbase.txt --execute` | skipped as `environment_read_unsupported`; combined triage run had `10,336` skips | Results depend on nondeterministic/system state not modeled in Remill tester state. `rdfsbase.txt` is fixed by Remill submodule commit `60cf744` and passes `1,040` rows with the tester's zero FS-base state; `rdsspd.txt`/`rdsspq.txt` are fixed by Remill submodule commit `2044e40` and pass `720`/`1,472` rows as disabled-CET no-ops. |
| Descriptor/control-state reads | `./build-release/remill-tester 3975WX/lar.txt 3975WX/lsl.txt --execute --stop-on-first-fail` | `105,997 passed, 0 failed, 246 skipped`; latest combined audit has `descriptor_state_unsupported=246` | Remill submodule commits `4b46adc`/`ad3f883` cover fixed 3975WX user code/data selectors and the stable low 16 bits for `LAR` selector `0x50` with 16-bit destinations. The remaining `246` rows use wider `LAR`/`LSL` variable per-CPU/system descriptor selectors (`0x50`/`0x51`) whose descriptor-table limit/access state is not serialized. `smsw.txt` is fixed by Remill submodule commit `7faf30b` and passes `1,776` rows with the corpus-observed machine-status word model. |

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
