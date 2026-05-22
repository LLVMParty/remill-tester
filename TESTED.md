# Remill Tester Coverage Notes

This file records concrete tester runs so failures can be distinguished from untested work.

## Current green full-file runs

All commands below completed with `execution_failed=0` and `execution_skipped=0` unless noted.

| Corpus | Command | Result |
|---|---|---|
| `3975WX/not.txt` | `./build/remill-tester 3975WX/not.txt --execute` | 2,632 passed |
| `3975WX/xor.txt` | `./build/remill-tester 3975WX/xor.txt --execute --stop-on-first-fail` | 114,672 passed |
| `3975WX/add.txt` | `./build/remill-tester 3975WX/add.txt --execute --stop-on-first-fail` | 117,756 passed |
| `3975WX/adc.txt` | `./build/remill-tester 3975WX/adc.txt --execute --stop-on-first-fail` | 127,260 passed |
| `3975WX/cmp.txt` | `./build/remill-tester 3975WX/cmp.txt --execute --stop-on-first-fail` | 12,432 passed |
| `3975WX/and.txt` | `./build/remill-tester 3975WX/and.txt --execute --stop-on-first-fail` | 94,100 passed |
| `3975WX/or.txt` | `./build/remill-tester 3975WX/or.txt --execute --stop-on-first-fail` | 105,276 passed |
| `3975WX/sub.txt` | `./build/remill-tester 3975WX/sub.txt --execute --stop-on-first-fail` | 117,928 passed |
| `3975WX/sbb.txt` | `./build/remill-tester 3975WX/sbb.txt --execute --stop-on-first-fail` | 126,308 passed |
| `3975WX/test.txt` | `./build/remill-tester 3975WX/test.txt --execute --stop-on-first-fail` | 10,672 passed |
| `3975WX/inc.txt` | `./build/remill-tester 3975WX/inc.txt --execute --stop-on-first-fail` | 3,064 passed |
| `3975WX/dec.txt` | `./build/remill-tester 3975WX/dec.txt --execute --stop-on-first-fail` | 3,288 passed |
| `3975WX/shl.txt` | `./build/remill-tester 3975WX/shl.txt --execute --stop-on-first-fail` | 24,018 passed |
| `3975WX/shr.txt` | `./build/remill-tester 3975WX/shr.txt --execute --stop-on-first-fail` | 24,110 passed |
| `3975WX/sar.txt` | `./build/remill-tester 3975WX/sar.txt --execute --stop-on-first-fail` | 28,274 passed |
| `3975WX/addps.txt` | `./build/remill-tester 3975WX/addps.txt --execute --stop-on-first-fail` | 63,920 passed |
| `3975WX/addpd.txt` | `./build/remill-tester 3975WX/addpd.txt --execute --stop-on-first-fail` | 63,088 passed |
| `3975WX/addsd.txt` | `./build/remill-tester 3975WX/addsd.txt --execute --stop-on-first-fail` | 61,224 passed |
| `3975WX/addss.txt` | `./build/remill-tester 3975WX/addss.txt --execute --stop-on-first-fail` | 60,312 passed |
| `3975WX/addsubpd.txt` | `./build/remill-tester 3975WX/addsubpd.txt --execute --stop-on-first-fail` | 62,272 passed |
| `3975WX/addsubps.txt` | `./build/remill-tester 3975WX/addsubps.txt --execute --stop-on-first-fail` | 63,472 passed |
| `3975WX/andpd.txt` | `./build/remill-tester 3975WX/andpd.txt --execute --stop-on-first-fail` | 54,272 passed |
| `3975WX/andps.txt` | `./build/remill-tester 3975WX/andps.txt --execute --stop-on-first-fail` | 52,048 passed |
| `3975WX/andnpd.txt` | `./build/remill-tester 3975WX/andnpd.txt --execute --stop-on-first-fail` | 55,072 passed |
| `3975WX/andnps.txt` | `./build/remill-tester 3975WX/andnps.txt --execute --stop-on-first-fail` | 54,288 passed |
| `3975WX/maxps.txt` | `./build/remill-tester 3975WX/maxps.txt --execute --stop-on-first-fail` | 61,824 passed |
| `3975WX/minps.txt` | `./build/remill-tester 3975WX/minps.txt --execute --stop-on-first-fail` | 61,272 passed |
| `3975WX/div.txt` | `./build/remill-tester 3975WX/div.txt --execute --stop-on-first-fail` | 7,292 passed, including expected exceptions |
| `3975WX/idiv.txt` | `./build/remill-tester 3975WX/idiv.txt --execute --stop-on-first-fail` | 6,933 passed, including expected exceptions |
| `3975WX/imul.txt` | `./build/remill-tester 3975WX/imul.txt --execute --stop-on-first-fail` | 998,329 passed |
| `3975WX/mul.txt` | `./build/remill-tester 3975WX/mul.txt --execute --stop-on-first-fail` | 7,974 passed |
| `3975WX/neg.txt` | `./build/remill-tester 3975WX/neg.txt --execute --stop-on-first-fail` | 3,240 passed |
| `3975WX/bsf.txt` | `./build/remill-tester 3975WX/bsf.txt --execute --stop-on-first-fail` | 37,048 passed |
| `3975WX/bsr.txt` | `./build/remill-tester 3975WX/bsr.txt --execute --stop-on-first-fail` | 38,624 passed |
| `3975WX/bt.txt` | `./build/remill-tester 3975WX/bt.txt --execute --stop-on-first-fail` | 1,536 passed |
| `3975WX/bts.txt` | `./build/remill-tester 3975WX/bts.txt --execute --stop-on-first-fail` | 77,696 passed |
| `3975WX/btr.txt` | `./build/remill-tester 3975WX/btr.txt --execute --stop-on-first-fail` | 76,896 passed |
| `3975WX/btc.txt` | `./build/remill-tester 3975WX/btc.txt --execute --stop-on-first-fail` | 78,984 passed |
| `3975WX/bswap.txt` | `./build/remill-tester 3975WX/bswap.txt --execute --stop-on-first-fail` | 2,224 passed |
| `3975WX/rol.txt` | `./build/remill-tester 3975WX/rol.txt --execute --stop-on-first-fail` | 29,018 passed |
| `3975WX/ror.txt` | `./build/remill-tester 3975WX/ror.txt --execute --stop-on-first-fail` | 28,157 passed |
| `3975WX/rcl.txt` | `./build/remill-tester 3975WX/rcl.txt --execute --stop-on-first-fail` | 32,090 passed |
| `3975WX/rcr.txt` | `./build/remill-tester 3975WX/rcr.txt --execute --stop-on-first-fail` | 31,490 passed |

## Current limited smoke runs

| Corpus | Command | Result |
|---|---|---|
| `3975WX/lea.txt` | `./build/remill-tester 3975WX/lea.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |
| `3975WX/mov.txt` | `./build/remill-tester 3975WX/mov.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |
| `3975WX/cmovz.txt` | `./build/remill-tester 3975WX/cmovz.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |
| `3975WX/cmovnz.txt` | `./build/remill-tester 3975WX/cmovnz.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |
| `3975WX/setz.txt` | `./build/remill-tester 3975WX/setz.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |
| `3975WX/setnz.txt` | `./build/remill-tester 3975WX/setnz.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |
| `3975WX/xchg.txt` | `./build/remill-tester 3975WX/xchg.txt --execute --limit-states 100 --stop-on-first-fail` | 100 passed |

## Current known mismatches

No current full-file mismatches are tracked here. Unsupported or unimplemented rows are reported as skips with explicit `summary.skip_reason` lines.

## Notes

- Logical instruction `AF` differences are masked using XED undefined flag metadata.
- SHL/SHR/SAR dynamically mask `CF` when the effective count is at least the operand width because the carry flag is architecturally undefined in that case.
- Shift/rotate flags are not compared for dynamic no-op counts unless the input row explicitly provides initial flags to verify preservation.
- LEA's address-generation operand is not treated as a real memory read/write, so it does not require `mem[...]` oracle cells.
