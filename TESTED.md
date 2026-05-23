# Remill Tester Coverage Notes

This file records concrete tester runs so failures can be distinguished from untested work.

## Current green full-file runs

All commands below completed with `execution_failed=0` and `execution_skipped=0` unless noted. Large sweeps should use the Release build (`build-release`) when available; earlier entries using `build` are equivalent Debug-build evidence.

| Corpus | Command | Result |
|---|---|---|
| `3975WX/aesdec.txt` | `./build-release/remill-tester 3975WX/aesdec.txt --execute --stop-on-first-fail` | 64,488 passed |
| `3975WX/aesdeclast.txt` | `./build-release/remill-tester 3975WX/aesdeclast.txt --execute --stop-on-first-fail` | 64,552 passed |
| `3975WX/aesenc.txt` | `./build-release/remill-tester 3975WX/aesenc.txt --execute --stop-on-first-fail` | 64,664 passed |
| `3975WX/aesenclast.txt` | `./build-release/remill-tester 3975WX/aesenclast.txt --execute --stop-on-first-fail` | 64,632 passed |
| `3975WX/aesimc.txt` | `./build-release/remill-tester 3975WX/aesimc.txt --execute --stop-on-first-fail` | 64,320 passed |
| `3975WX/aeskeygenassist.txt` | `./build-release/remill-tester 3975WX/aeskeygenassist.txt --execute --stop-on-first-fail` | 580,168 passed |
| `3975WX/not.txt` | `./build/remill-tester 3975WX/not.txt --execute` | 2,632 passed |
| `3975WX/xor.txt` | `./build/remill-tester 3975WX/xor.txt --execute --stop-on-first-fail` | 114,672 passed |
| `3975WX/add.txt` | `./build/remill-tester 3975WX/add.txt --execute --stop-on-first-fail` | 117,756 passed |
| `3975WX/adc.txt` | `./build/remill-tester 3975WX/adc.txt --execute --stop-on-first-fail` | 127,260 passed |
| `3975WX/adcx.txt` | `./build-release/remill-tester 3975WX/adcx.txt --execute --stop-on-first-fail` | 49,216 passed |
| `3975WX/adox.txt` | `./build-release/remill-tester 3975WX/adox.txt --execute --stop-on-first-fail` | 49,552 passed |
| `3975WX/cmp.txt` | `./build/remill-tester 3975WX/cmp.txt --execute --stop-on-first-fail` | 12,432 passed |
| `3975WX/and.txt` | `./build/remill-tester 3975WX/and.txt --execute --stop-on-first-fail` | 94,100 passed |
| `3975WX/or.txt` | `./build/remill-tester 3975WX/or.txt --execute --stop-on-first-fail` | 105,276 passed |
| `3975WX/sub.txt` | `./build/remill-tester 3975WX/sub.txt --execute --stop-on-first-fail` | 117,928 passed |
| `3975WX/sbb.txt` | `./build/remill-tester 3975WX/sbb.txt --execute --stop-on-first-fail` | 126,308 passed |
| `3975WX/sha1msg1.txt` | `./build-release/remill-tester 3975WX/sha1msg1.txt --execute --stop-on-first-fail` | 63,904 passed |
| `3975WX/sha1msg2.txt` | `./build-release/remill-tester 3975WX/sha1msg2.txt --execute --stop-on-first-fail` | 64,432 passed |
| `3975WX/sha1nexte.txt` | `./build-release/remill-tester 3975WX/sha1nexte.txt --execute --stop-on-first-fail` | 60,704 passed |
| `3975WX/sha1rnds4.txt` | `./build-release/remill-tester 3975WX/sha1rnds4.txt --execute --stop-on-first-fail` | 580,736 passed |
| `3975WX/sha256msg1.txt` | `./build-release/remill-tester 3975WX/sha256msg1.txt --execute --stop-on-first-fail` | 64,752 passed |
| `3975WX/sha256msg2.txt` | `./build-release/remill-tester 3975WX/sha256msg2.txt --execute --stop-on-first-fail` | 64,560 passed |
| `3975WX/sha256rnds2.txt` | `./build-release/remill-tester 3975WX/sha256rnds2.txt --execute --stop-on-first-fail` | 65,486 passed |
| `3975WX/rdfsbase.txt` | `./build-release/remill-tester 3975WX/rdfsbase.txt --execute --stop-on-first-fail` | 1,040 passed |
| `3975WX/test.txt` | `./build/remill-tester 3975WX/test.txt --execute --stop-on-first-fail` | 10,672 passed |
| `3975WX/inc.txt` | `./build/remill-tester 3975WX/inc.txt --execute --stop-on-first-fail` | 3,064 passed |
| `3975WX/dec.txt` | `./build/remill-tester 3975WX/dec.txt --execute --stop-on-first-fail` | 3,288 passed |
| `3975WX/shl.txt` | `./build/remill-tester 3975WX/shl.txt --execute --stop-on-first-fail` | 24,018 passed |
| `3975WX/shr.txt` | `./build/remill-tester 3975WX/shr.txt --execute --stop-on-first-fail` | 24,110 passed |
| `3975WX/sar.txt` | `./build/remill-tester 3975WX/sar.txt --execute --stop-on-first-fail` | 28,274 passed |
| `3975WX/shld.txt` | `./build-release/remill-tester 3975WX/shld.txt --execute --stop-on-first-fail` | 585,136 passed |
| `3975WX/shrd.txt` | `./build-release/remill-tester 3975WX/shrd.txt --execute --stop-on-first-fail` | 580,799 passed |
| `3975WX/addps.txt` | `./build/remill-tester 3975WX/addps.txt --execute --stop-on-first-fail` | 63,920 passed |
| `3975WX/addpd.txt` | `./build/remill-tester 3975WX/addpd.txt --execute --stop-on-first-fail` | 63,088 passed |
| `3975WX/addsd.txt` | `./build/remill-tester 3975WX/addsd.txt --execute --stop-on-first-fail` | 61,224 passed |
| `3975WX/addss.txt` | `./build/remill-tester 3975WX/addss.txt --execute --stop-on-first-fail` | 60,312 passed |
| `3975WX/addsubpd.txt` | `./build/remill-tester 3975WX/addsubpd.txt --execute --stop-on-first-fail` | 62,272 passed |
| `3975WX/addsubps.txt` | `./build/remill-tester 3975WX/addsubps.txt --execute --stop-on-first-fail` | 63,472 passed |
| `3975WX/haddpd.txt` | `./build-release/remill-tester 3975WX/haddpd.txt --execute --stop-on-first-fail` | 62,816 passed |
| `3975WX/haddps.txt` | `./build-release/remill-tester 3975WX/haddps.txt --execute --stop-on-first-fail` | 63,064 passed |
| `3975WX/hsubpd.txt` | `./build-release/remill-tester 3975WX/hsubpd.txt --execute --stop-on-first-fail` | 63,992 passed |
| `3975WX/hsubps.txt` | `./build-release/remill-tester 3975WX/hsubps.txt --execute --stop-on-first-fail` | 63,408 passed |
| `3975WX/subpd.txt` | `./build-release/remill-tester 3975WX/subpd.txt --execute --stop-on-first-fail` | 62,688 passed |
| `3975WX/subps.txt` | `./build-release/remill-tester 3975WX/subps.txt --execute --stop-on-first-fail` | 62,712 passed |
| `3975WX/subsd.txt` | `./build-release/remill-tester 3975WX/subsd.txt --execute --stop-on-first-fail` | 60,408 passed |
| `3975WX/subss.txt` | `./build-release/remill-tester 3975WX/subss.txt --execute --stop-on-first-fail` | 58,528 passed |
| `3975WX/mulpd.txt` | `./build-release/remill-tester 3975WX/mulpd.txt --execute --stop-on-first-fail` | 63,936 passed |
| `3975WX/mulps.txt` | `./build-release/remill-tester 3975WX/mulps.txt --execute --stop-on-first-fail` | 64,096 passed |
| `3975WX/mulsd.txt` | `./build-release/remill-tester 3975WX/mulsd.txt --execute --stop-on-first-fail` | 61,544 passed |
| `3975WX/mulss.txt` | `./build-release/remill-tester 3975WX/mulss.txt --execute --stop-on-first-fail` | 61,016 passed |
| `3975WX/divpd.txt` | `./build-release/remill-tester 3975WX/divpd.txt --execute --stop-on-first-fail` | 62,880 passed |
| `3975WX/divps.txt` | `./build-release/remill-tester 3975WX/divps.txt --execute --stop-on-first-fail` | 62,824 passed |
| `3975WX/divsd.txt` | `./build-release/remill-tester 3975WX/divsd.txt --execute --stop-on-first-fail` | 61,944 passed |
| `3975WX/divss.txt` | `./build-release/remill-tester 3975WX/divss.txt --execute --stop-on-first-fail` | 59,912 passed |
| `3975WX/sqrtpd.txt` | `./build-release/remill-tester 3975WX/sqrtpd.txt --execute --stop-on-first-fail` | 64,416 passed |
| `3975WX/sqrtps.txt` | `./build-release/remill-tester 3975WX/sqrtps.txt --execute --stop-on-first-fail` | 64,168 passed |
| `3975WX/sqrtsd.txt` | `./build-release/remill-tester 3975WX/sqrtsd.txt --execute --stop-on-first-fail` | 61,744 passed |
| `3975WX/sqrtss.txt` | `./build-release/remill-tester 3975WX/sqrtss.txt --execute --stop-on-first-fail` | 60,880 passed |
| `3975WX/maxpd.txt` | `./build-release/remill-tester 3975WX/maxpd.txt --execute --stop-on-first-fail` | 61,568 passed |
| `3975WX/maxsd.txt` | `./build-release/remill-tester 3975WX/maxsd.txt --execute --stop-on-first-fail` | 60,232 passed |
| `3975WX/maxss.txt` | `./build-release/remill-tester 3975WX/maxss.txt --execute --stop-on-first-fail` | 59,848 passed |
| `3975WX/minpd.txt` | `./build-release/remill-tester 3975WX/minpd.txt --execute --stop-on-first-fail` | 61,768 passed |
| `3975WX/minsd.txt` | `./build-release/remill-tester 3975WX/minsd.txt --execute --stop-on-first-fail` | 59,528 passed |
| `3975WX/minss.txt` | `./build-release/remill-tester 3975WX/minss.txt --execute --stop-on-first-fail` | 60,600 passed |
| `3975WX/andpd.txt` | `./build/remill-tester 3975WX/andpd.txt --execute --stop-on-first-fail` | 54,272 passed |
| `3975WX/andps.txt` | `./build/remill-tester 3975WX/andps.txt --execute --stop-on-first-fail` | 52,048 passed |
| `3975WX/andnpd.txt` | `./build/remill-tester 3975WX/andnpd.txt --execute --stop-on-first-fail` | 55,072 passed |
| `3975WX/andnps.txt` | `./build/remill-tester 3975WX/andnps.txt --execute --stop-on-first-fail` | 54,288 passed |
| `3975WX/orpd.txt` | `./build-release/remill-tester 3975WX/orpd.txt --execute --stop-on-first-fail` | 64,224 passed |
| `3975WX/orps.txt` | `./build-release/remill-tester 3975WX/orps.txt --execute --stop-on-first-fail` | 64,128 passed |
| `3975WX/xorpd.txt` | `./build-release/remill-tester 3975WX/xorpd.txt --execute --stop-on-first-fail` | 62,784 passed |
| `3975WX/xorps.txt` | `./build-release/remill-tester 3975WX/xorps.txt --execute --stop-on-first-fail` | 62,512 passed |
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
| `3975WX/mov.txt` | `./build/remill-tester 3975WX/mov.txt --execute --stop-on-first-fail` | 77,946 passed |
| `3975WX/lea.txt` | `./build/remill-tester 3975WX/lea.txt --execute --stop-on-first-fail` | 1,171,188 passed |
| `3975WX/xchg.txt` | `./build/remill-tester 3975WX/xchg.txt --execute --stop-on-first-fail` | 110,033 passed |
| `3975WX/cmpxchg.txt` | `./build-release/remill-tester 3975WX/cmpxchg.txt --execute --stop-on-first-fail` | 116,273 passed |
| `3975WX/xadd.txt` | `./build-release/remill-tester 3975WX/xadd.txt --execute --stop-on-first-fail` | 126,006 passed |
| `3975WX/movzx.txt` | `./build-release/remill-tester 3975WX/movzx.txt --execute --stop-on-first-fail` | 75,022 passed |
| `3975WX/movsx.txt` | `./build-release/remill-tester 3975WX/movsx.txt --execute --stop-on-first-fail` | 113,322 passed |
| `3975WX/movsxd.txt` | `./build-release/remill-tester 3975WX/movsxd.txt --execute --stop-on-first-fail` | 54,256 passed |
| `3975WX/movapd.txt` | `./build-release/remill-tester 3975WX/movapd.txt --execute --stop-on-first-fail` | 59,432 passed |
| `3975WX/movaps.txt` | `./build-release/remill-tester 3975WX/movaps.txt --execute --stop-on-first-fail` | 59,576 passed |
| `3975WX/movupd.txt` | `./build-release/remill-tester 3975WX/movupd.txt --execute --stop-on-first-fail` | 59,984 passed |
| `3975WX/movups.txt` | `./build-release/remill-tester 3975WX/movups.txt --execute --stop-on-first-fail` | 59,256 passed |
| `3975WX/movdqa.txt` | `./build-release/remill-tester 3975WX/movdqa.txt --execute --stop-on-first-fail` | 60,176 passed |
| `3975WX/movdqu.txt` | `./build-release/remill-tester 3975WX/movdqu.txt --execute --stop-on-first-fail` | 59,152 passed |
| `3975WX/movhlps.txt` | `./build-release/remill-tester 3975WX/movhlps.txt --execute --stop-on-first-fail` | 59,288 passed |
| `3975WX/movlhps.txt` | `./build-release/remill-tester 3975WX/movlhps.txt --execute --stop-on-first-fail` | 60,504 passed |
| `3975WX/movshdup.txt` | `./build-release/remill-tester 3975WX/movshdup.txt --execute --stop-on-first-fail` | 59,568 passed |
| `3975WX/movsldup.txt` | `./build-release/remill-tester 3975WX/movsldup.txt --execute --stop-on-first-fail` | 58,840 passed |
| `3975WX/movddup.txt` | `./build-release/remill-tester 3975WX/movddup.txt --execute --stop-on-first-fail` | 59,144 passed |
| `3975WX/movss.txt` | `./build-release/remill-tester 3975WX/movss.txt --execute --stop-on-first-fail` | 58,376 passed |
| `3975WX/movmskpd.txt` | `./build-release/remill-tester 3975WX/movmskpd.txt --execute --stop-on-first-fail` | 25,600 passed |
| `3975WX/movmskps.txt` | `./build-release/remill-tester 3975WX/movmskps.txt --execute --stop-on-first-fail` | 26,368 passed |
| `3975WX/unpckhpd.txt` | `./build-release/remill-tester 3975WX/unpckhpd.txt --execute --stop-on-first-fail` | 58,816 passed |
| `3975WX/unpckhps.txt` | `./build-release/remill-tester 3975WX/unpckhps.txt --execute --stop-on-first-fail` | 59,984 passed |
| `3975WX/unpcklpd.txt` | `./build-release/remill-tester 3975WX/unpcklpd.txt --execute --stop-on-first-fail` | 58,032 passed |
| `3975WX/unpcklps.txt` | `./build-release/remill-tester 3975WX/unpcklps.txt --execute --stop-on-first-fail` | 58,880 passed |
| `3975WX/shufpd.txt` | `./build-release/remill-tester 3975WX/shufpd.txt --execute --stop-on-first-fail` | 538,624 passed |
| `3975WX/shufps.txt` | `./build-release/remill-tester 3975WX/shufps.txt --execute --stop-on-first-fail` | 525,376 passed |
| `3975WX/cvtdq2pd.txt` | `./build-release/remill-tester 3975WX/cvtdq2pd.txt --execute --stop-on-first-fail` | 51,024 passed |
| `3975WX/cvtdq2ps.txt` | `./build-release/remill-tester 3975WX/cvtdq2ps.txt --execute --stop-on-first-fail` | 62,512 passed |
| `3975WX/cvtpd2dq.txt` | `./build-release/remill-tester 3975WX/cvtpd2dq.txt --execute --stop-on-first-fail` | 48,360 passed |
| `3975WX/cvtpd2ps.txt` | `./build-release/remill-tester 3975WX/cvtpd2ps.txt --execute --stop-on-first-fail` | 42,888 passed |
| `3975WX/cvtps2dq.txt` | `./build-release/remill-tester 3975WX/cvtps2dq.txt --execute --stop-on-first-fail` | 42,984 passed |
| `3975WX/cvtps2pd.txt` | `./build-release/remill-tester 3975WX/cvtps2pd.txt --execute --stop-on-first-fail` | 47,928 passed |
| `3975WX/cvtsd2si.txt` | `./build-release/remill-tester 3975WX/cvtsd2si.txt --execute --stop-on-first-fail` | 45,312 passed |
| `3975WX/cvtsd2ss.txt` | `./build-release/remill-tester 3975WX/cvtsd2ss.txt --execute --stop-on-first-fail` | 58,360 passed |
| `3975WX/cvtsi2sd.txt` | `./build-release/remill-tester 3975WX/cvtsi2sd.txt --execute --stop-on-first-fail` | 112,128 passed |
| `3975WX/cvtsi2ss.txt` | `./build-release/remill-tester 3975WX/cvtsi2ss.txt --execute --stop-on-first-fail` | 116,480 passed |
| `3975WX/cvtss2sd.txt` | `./build-release/remill-tester 3975WX/cvtss2sd.txt --execute --stop-on-first-fail` | 53,552 passed |
| `3975WX/cvtss2si.txt` | `./build-release/remill-tester 3975WX/cvtss2si.txt --execute --stop-on-first-fail` | 34,048 passed |
| `3975WX/cvttpd2dq.txt` | `./build-release/remill-tester 3975WX/cvttpd2dq.txt --execute --stop-on-first-fail` | 48,144 passed |
| `3975WX/cvttps2dq.txt` | `./build-release/remill-tester 3975WX/cvttps2dq.txt --execute --stop-on-first-fail` | 40,856 passed |
| `3975WX/cvttsd2si.txt` | `./build-release/remill-tester 3975WX/cvttsd2si.txt --execute --stop-on-first-fail` | 45,568 passed |
| `3975WX/cvttss2si.txt` | `./build-release/remill-tester 3975WX/cvttss2si.txt --execute --stop-on-first-fail` | 32,512 passed |
| `3975WX/roundpd.txt` | `./build-release/remill-tester 3975WX/roundpd.txt --execute --stop-on-first-fail` | 454,512 passed |
| `3975WX/roundps.txt` | `./build-release/remill-tester 3975WX/roundps.txt --execute --stop-on-first-fail` | 479,064 passed |
| `3975WX/roundsd.txt` | `./build-release/remill-tester 3975WX/roundsd.txt --execute --stop-on-first-fail` | 506,000 passed |
| `3975WX/roundss.txt` | `./build-release/remill-tester 3975WX/roundss.txt --execute --stop-on-first-fail` | 513,920 passed |
| `3975WX/dppd.txt` | `./build-release/remill-tester 3975WX/dppd.txt --execute --stop-on-first-fail` | 353,856 passed |
| `3975WX/dpps.txt` | `./build-release/remill-tester 3975WX/dpps.txt --execute --stop-on-first-fail` | 350,928 passed |
| `3975WX/mpsadbw.txt` | `./build-release/remill-tester 3975WX/mpsadbw.txt --execute --stop-on-first-fail` | 470,232 passed |
| `3975WX/insertq.txt` | `./build-release/remill-tester 3975WX/insertq.txt --execute --stop-on-first-fail` | 3,741,176 passed |
| `3975WX/cmppd.txt` | `./build-release/remill-tester 3975WX/cmppd.txt --execute --stop-on-first-fail` | 523,512 passed |
| `3975WX/cmpps.txt` | `./build-release/remill-tester 3975WX/cmpps.txt --execute --stop-on-first-fail` | 497,792 passed |
| `3975WX/comisd.txt` | `./build-release/remill-tester 3975WX/comisd.txt --execute --stop-on-first-fail` | 2,272 passed |
| `3975WX/comiss.txt` | `./build-release/remill-tester 3975WX/comiss.txt --execute --stop-on-first-fail` | 2,272 passed |
| `3975WX/ucomisd.txt` | `./build-release/remill-tester 3975WX/ucomisd.txt --execute --stop-on-first-fail` | 2,272 passed |
| `3975WX/ucomiss.txt` | `./build-release/remill-tester 3975WX/ucomiss.txt --execute --stop-on-first-fail` | 2,272 passed |
| `3975WX/blendpd.txt` | `./build-release/remill-tester 3975WX/blendpd.txt --execute --stop-on-first-fail` | 529,968 passed |
| `3975WX/blendps.txt` | `./build-release/remill-tester 3975WX/blendps.txt --execute --stop-on-first-fail` | 537,288 passed |
| `3975WX/blendvpd.txt` | `./build-release/remill-tester 3975WX/blendvpd.txt --execute --stop-on-first-fail` | 61,346 passed |
| `3975WX/blendvps.txt` | `./build-release/remill-tester 3975WX/blendvps.txt --execute --stop-on-first-fail` | 60,230 passed |
| `3975WX/extractps.txt` | `./build-release/remill-tester 3975WX/extractps.txt --execute --stop-on-first-fail` | 124,160 passed |
| `3975WX/insertps.txt` | `./build-release/remill-tester 3975WX/insertps.txt --execute --stop-on-first-fail` | 405,160 passed |
| `3975WX/cbw.txt` | `./build-release/remill-tester 3975WX/cbw.txt --execute --stop-on-first-fail` | 27 passed |
| `3975WX/cwde.txt` | `./build-release/remill-tester 3975WX/cwde.txt --execute --stop-on-first-fail` | 42 passed |
| `3975WX/cdqe.txt` | `./build-release/remill-tester 3975WX/cdqe.txt --execute --stop-on-first-fail` | 87 passed |
| `3975WX/cwd.txt` | `./build-release/remill-tester 3975WX/cwd.txt --execute --stop-on-first-fail` | 32 passed |
| `3975WX/cdq.txt` | `./build-release/remill-tester 3975WX/cdq.txt --execute --stop-on-first-fail` | 63 passed |
| `3975WX/cqo.txt` | `./build-release/remill-tester 3975WX/cqo.txt --execute --stop-on-first-fail` | 127 passed |
| `3975WX/clc.txt` | `./build-release/remill-tester 3975WX/clc.txt --execute --stop-on-first-fail` | 1 passed |
| `3975WX/stc.txt` | `./build-release/remill-tester 3975WX/stc.txt --execute --stop-on-first-fail` | 1 passed |
| `3975WX/cmc.txt` | `./build-release/remill-tester 3975WX/cmc.txt --execute --stop-on-first-fail` | 2 passed |
| `3975WX/cld.txt` | `./build-release/remill-tester 3975WX/cld.txt --execute --stop-on-first-fail` | 1 passed |
| `3975WX/std.txt` | `./build-release/remill-tester 3975WX/std.txt --execute --stop-on-first-fail` | 1 passed |
| `3975WX/lahf.txt` | `./build-release/remill-tester 3975WX/lahf.txt --execute --stop-on-first-fail` | 13 passed |
| `3975WX/sahf.txt` | `./build-release/remill-tester 3975WX/sahf.txt --execute --stop-on-first-fail` | 8 passed |
| `3975WX/int.txt` | `./build-release/remill-tester 3975WX/int.txt --execute --stop-on-first-fail` | 32 passed, including expected exceptions |
| `3975WX/int3.txt` | `./build-release/remill-tester 3975WX/int3.txt --execute --stop-on-first-fail` | 3 passed, including expected exceptions |
| `3975WX/setb.txt` | `./build-release/remill-tester 3975WX/setb.txt --execute --stop-on-first-fail` | 128 passed |
| `3975WX/setbe.txt` | `./build-release/remill-tester 3975WX/setbe.txt --execute --stop-on-first-fail` | 160 passed |
| `3975WX/setl.txt` | `./build-release/remill-tester 3975WX/setl.txt --execute --stop-on-first-fail` | 180 passed |
| `3975WX/setle.txt` | `./build-release/remill-tester 3975WX/setle.txt --execute --stop-on-first-fail` | 180 passed |
| `3975WX/setnb.txt` | `./build-release/remill-tester 3975WX/setnb.txt --execute --stop-on-first-fail` | 128 passed |
| `3975WX/setnbe.txt` | `./build-release/remill-tester 3975WX/setnbe.txt --execute --stop-on-first-fail` | 180 passed |
| `3975WX/setnl.txt` | `./build-release/remill-tester 3975WX/setnl.txt --execute --stop-on-first-fail` | 160 passed |
| `3975WX/setnle.txt` | `./build-release/remill-tester 3975WX/setnle.txt --execute --stop-on-first-fail` | 144 passed |
| `3975WX/setno.txt` | `./build-release/remill-tester 3975WX/setno.txt --execute --stop-on-first-fail` | 148 passed |
| `3975WX/setnp.txt` | `./build-release/remill-tester 3975WX/setnp.txt --execute --stop-on-first-fail` | 144 passed |
| `3975WX/setns.txt` | `./build-release/remill-tester 3975WX/setns.txt --execute --stop-on-first-fail` | 164 passed |
| `3975WX/setnz.txt` | `./build-release/remill-tester 3975WX/setnz.txt --execute --stop-on-first-fail` | 180 passed |
| `3975WX/seto.txt` | `./build-release/remill-tester 3975WX/seto.txt --execute --stop-on-first-fail` | 160 passed |
| `3975WX/setp.txt` | `./build-release/remill-tester 3975WX/setp.txt --execute --stop-on-first-fail` | 160 passed |
| `3975WX/sets.txt` | `./build-release/remill-tester 3975WX/sets.txt --execute --stop-on-first-fail` | 128 passed |
| `3975WX/setz.txt` | `./build-release/remill-tester 3975WX/setz.txt --execute --stop-on-first-fail` | 164 passed |
| `3975WX/cmovb.txt` | `./build-release/remill-tester 3975WX/cmovb.txt --execute --stop-on-first-fail` | 56,472 passed |
| `3975WX/cmovbe.txt` | `./build-release/remill-tester 3975WX/cmovbe.txt --execute --stop-on-first-fail` | 56,672 passed |
| `3975WX/cmovl.txt` | `./build-release/remill-tester 3975WX/cmovl.txt --execute --stop-on-first-fail` | 56,840 passed |
| `3975WX/cmovle.txt` | `./build-release/remill-tester 3975WX/cmovle.txt --execute --stop-on-first-fail` | 56,936 passed |
| `3975WX/cmovnb.txt` | `./build-release/remill-tester 3975WX/cmovnb.txt --execute --stop-on-first-fail` | 56,352 passed |
| `3975WX/cmovnbe.txt` | `./build-release/remill-tester 3975WX/cmovnbe.txt --execute --stop-on-first-fail` | 57,040 passed |
| `3975WX/cmovnl.txt` | `./build-release/remill-tester 3975WX/cmovnl.txt --execute --stop-on-first-fail` | 56,848 passed |
| `3975WX/cmovnle.txt` | `./build-release/remill-tester 3975WX/cmovnle.txt --execute --stop-on-first-fail` | 57,024 passed |
| `3975WX/cmovno.txt` | `./build-release/remill-tester 3975WX/cmovno.txt --execute --stop-on-first-fail` | 56,624 passed |
| `3975WX/cmovnp.txt` | `./build-release/remill-tester 3975WX/cmovnp.txt --execute --stop-on-first-fail` | 56,496 passed |
| `3975WX/cmovns.txt` | `./build-release/remill-tester 3975WX/cmovns.txt --execute --stop-on-first-fail` | 56,304 passed |
| `3975WX/cmovnz.txt` | `./build-release/remill-tester 3975WX/cmovnz.txt --execute --stop-on-first-fail` | 56,672 passed |
| `3975WX/cmovo.txt` | `./build-release/remill-tester 3975WX/cmovo.txt --execute --stop-on-first-fail` | 56,520 passed |
| `3975WX/cmovp.txt` | `./build-release/remill-tester 3975WX/cmovp.txt --execute --stop-on-first-fail` | 56,352 passed |
| `3975WX/cmovs.txt` | `./build-release/remill-tester 3975WX/cmovs.txt --execute --stop-on-first-fail` | 56,704 passed |
| `3975WX/cmovz.txt` | `./build-release/remill-tester 3975WX/cmovz.txt --execute --stop-on-first-fail` | 56,568 passed |
| `3975WX/andn.txt` | `./build-release/remill-tester 3975WX/andn.txt --execute --stop-on-first-fail` | 744,816 passed |
| `3975WX/bzhi.txt` | `./build-release/remill-tester 3975WX/bzhi.txt --execute --stop-on-first-fail` | 766,328 passed |
| `3975WX/lzcnt.txt` | `./build-release/remill-tester 3975WX/lzcnt.txt --execute --stop-on-first-fail` | 34,400 passed |
| `3975WX/tzcnt.txt` | `./build-release/remill-tester 3975WX/tzcnt.txt --execute --stop-on-first-fail` | 34,104 passed |
| `3975WX/blsi.txt` | `./build-release/remill-tester 3975WX/blsi.txt --execute --stop-on-first-fail` | 49,464 passed |
| `3975WX/blsr.txt` | `./build-release/remill-tester 3975WX/blsr.txt --execute --stop-on-first-fail` | 47,976 passed |
| `3975WX/blsmsk.txt` | `./build-release/remill-tester 3975WX/blsmsk.txt --execute --stop-on-first-fail` | 43,848 passed |
| `3975WX/bextr.txt` | `./build-release/remill-tester 3975WX/bextr.txt --execute --stop-on-first-fail` | 647,608 passed |
| `3975WX/rorx.txt` | `./build-release/remill-tester 3975WX/rorx.txt --execute --stop-on-first-fail` | 405,456 passed |
| `3975WX/mulx.txt` | `./build-release/remill-tester 3975WX/mulx.txt --execute --stop-on-first-fail` | 1,520,573 passed |
| `3975WX/shlx.txt` | `./build-release/remill-tester 3975WX/shlx.txt --execute --stop-on-first-fail` | 746,440 passed |
| `3975WX/sarx.txt` | `./build-release/remill-tester 3975WX/sarx.txt --execute --stop-on-first-fail` | 770,320 passed |
| `3975WX/shrx.txt` | `./build-release/remill-tester 3975WX/shrx.txt --execute --stop-on-first-fail` | 738,888 passed |
| `3975WX/rol.txt` | `./build/remill-tester 3975WX/rol.txt --execute --stop-on-first-fail` | 29,018 passed |
| `3975WX/ror.txt` | `./build/remill-tester 3975WX/ror.txt --execute --stop-on-first-fail` | 28,157 passed |
| `3975WX/rcl.txt` | `./build/remill-tester 3975WX/rcl.txt --execute --stop-on-first-fail` | 32,090 passed |
| `3975WX/rcr.txt` | `./build/remill-tester 3975WX/rcr.txt --execute --stop-on-first-fail` | 31,490 passed |

## Current limited smoke runs

No limited-only smoke runs are currently tracked; entries are promoted to the full-file table after full corpus execution.

## Current known mismatches

No current full-file mismatches are tracked here. Unsupported or unimplemented rows are reported as skips with explicit `summary.skip_reason` lines. `xlat.txt` currently skips all 16 rows as `memory_state_missing` because the raw corpus does not serialize the table memory contents.

## Notes

- Logical instruction `AF` differences are masked using XED undefined flag metadata.
- SHL/SHR/SAR dynamically mask `CF` when the effective count is at least the operand width because the carry flag is architecturally undefined in that case.
- Shift/rotate flags are not compared for dynamic no-op counts unless the input row explicitly provides initial flags to verify preservation.
- LEA's address-generation operand is not treated as a real memory read/write, so it does not require `mem[...]` oracle cells.
