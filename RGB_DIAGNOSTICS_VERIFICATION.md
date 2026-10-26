# RGB calibration diagnostics, 0.1.8-dev

Superseded diagnosis: the user retested 0.1.8-dev and still observed 2/4/6/0. The confirmed software cause is an Arduino `word` macro collision, fixed in 0.1.9-dev; see [RGB_WORD_FIX_VERIFICATION.md](RGB_WORD_FIX_VERIFICATION.md). The protocol change below did not resolve that fault.

2026-10-21. User reports RGB0, Red: R=2, G=4, B=6, Clear=0 while calibrating.

Confirmed cause of rejection: Clear is below the existing minimum of 10. This does not establish the physical reason for that reading. No physical sensor measurements or uploads were performed by the agent.

Two changes:

- `PuppyBotRGB.h` reads C/R/G/B as four checked 16-bit word reads, using the same command bytes as the installed High-PuppyBot6 Adafruit driver. It no longer uses the auto-increment eight-byte burst. Scheduled oscillator/integration waits remain, so there is no getRawData/delay call in the new read/recovery path. Protocol compatibility is a hypothesis to test on the user's board, not a confirmed hardware root cause.
- Calibration keeps the rejected RGBC tuple in RAM, distinguishes zero RGB, low Clear and saturation, and displays the failed tuple under FAIL instead of continuing to show unrelated live values. Retry or selecting another color restores LIVE. No calibration threshold is relaxed, and no rejected reference replaces the previous record.

IMU, classifier, optical settings and storage implementation were compared with the pre-change backup and are unchanged. Flash schema remains v2; valid data saved by 0.1.7-dev is compatible. The user's SW1-calibration sketch and its setup1/loop1 remain unchanged.

Validation:

- RGB reader tests: High/Adafruit command bytes, both buses, startup/cadence/reconnect, short-transfer rejection, stalled ADC, timestamp wrap, cross-core cache, zero integration delay calls and monitor display pass. The host double models word reads; it does not reproduce the physical cause of the reported values.
- Calibration tests reproduce rejection of 2/4/6/0 with Clear-specific feedback, preserve the failed tuple after live recovery, preserve the prior reference, and verify frozen display, portrait bounds and LIVE restoration without ghost pixels. Existing SW1, Save/Load, CRC, partial profiles, audio and display tests also pass.
- Classifier: 10,048 differential inputs against the actual High Check_Color still pass, along with validity and persistence tests.
- Generated calibration-clear-zero.png was visually inspected: FAIL column shows 2/4/6/0 and Clear <10: retry.
- Actual user sketch compiles for logistic15u:rp2040:logistic15u, using installed 0.1.8-dev: 113764 bytes flash, 13168 bytes global RAM. Log: `.arduino-test/compile-user-rgb-diagnostics.log`.

Backup: `.arduino-test/before-rgb-invalid-diagnostics/`. No GitHub push or firmware upload. Next required physical check: upload the compiled user sketch, retry Red on RGB0 with the same placement, and inspect the exact FAIL values if rejection persists.
