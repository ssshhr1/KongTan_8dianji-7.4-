# Task Plan: Start Point And Line Trajectory

## Phases

- [x] Phase 1: Inspect current coordinate and trajectory flow
  - [x] Locate dialog controls and handlers for coordinate input.
  - [x] Locate motion/path generation code.
- [x] Phase 2: Implement feature
  - [x] Add or connect start-point input handling.
  - [x] Compute the next point pose for a straight-line segment.
  - [x] Integrate straight-line trajectory generation with existing output.
- [x] Phase 3: Verify
  - [x] Build or run available checks.
  - [x] Summarize changed files and behavior.

## Decisions

- 2026-07-10: Keep changes scoped to the existing MFC project patterns.

## Notes

- User request: input a starting coordinate point, judge the next point pose, and complete a straight-line trajectory.
- Verification note: MSBuild Debug|x64 reached resource compilation and failed at existing `KongTan8dianji.rc(98)` with `RC2104`, not in the modified C++ trajectory code.
