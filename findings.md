# Findings

## Investigation

- 2026-07-16: `MotorIds` contained only motors 1-6; motors 7 and 8 were commented out, so all third-segment handlers returned at their availability guard.
- 2026-07-16: Third-segment down/up/reset handlers already command motors 7 and 8 as an opposing pair and use `motor_V78`/`motor_S78`.
- 2026-07-16: The default third-segment step is 10,000 counts; the existing but disabled soft-limit design for motors 7 and 8 is +/-50,000 counts.

## Bugs

- 2026-07-16: Motors 7 and 8 were excluded from initialization, zero capture, reset, shutdown waiting, and third-segment motion. Status: fixed by including them in `MotorIds`.
- 2026-07-16: Motors 7 and 8 had no active soft-limit configuration. Status: fixed with the existing intended +/-50,000-count limits.
- 2026-07-16: Full Debug x64 build is blocked by pre-existing `KongTan8dianji.rc(98): RC2104 undefined keyword or key name: MS`; the changed C++ translation unit compiled successfully. Status: unrelated/open.

## Architecture

- `MotorIds` is the central configured-motor set used by initialization, zero capture, soft-limit application, reset, availability guards, and shutdown waiting.
- `EnableMotorBus()` enables the bus; membership in `MotorIds` determines which amplifiers the application subsequently initializes and commands.
