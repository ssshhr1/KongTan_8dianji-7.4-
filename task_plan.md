# Task Plan: Enable All Motors and Third-Segment Motion

## Phases

- [x] Phase 1: Inspect project control flow
  - [x] Locate motor enable/disable logic.
  - [x] Locate third-segment motion and soft-limit logic.
- [x] Phase 2: Implement coordinated changes
  - [x] Enable every configured motor.
  - [x] Include the third segment in commanded motion.
  - [x] Adjust only the soft limits required by the new motion range.
- [x] Phase 3: Verify
  - [x] Review diffs and control-path consistency.
  - [x] Run available build or static checks.

## Decisions

- 2026-07-16: Preserve existing control architecture and make the smallest coherent change because this is hardware-control code.
- 2026-07-16: Use +/-50,000 counts for motors 7 and 8 because the code already designated that range and the default manual step is 10,000 counts.
- 2026-07-16: Treat the existing resource-compiler error at `KongTan8dianji.rc(98)` as outside this change; C++ compilation completed successfully before that stage.

## Notes

- Do not broaden physical travel ranges beyond what the existing configuration and commanded trajectory justify.
