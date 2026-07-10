# Findings

## Architecture

- 2026-07-10: Investigation pending.
- 2026-07-10: Main coordinate and trajectory logic appears in `KongTan_8dianjiDlg.cpp`, especially around existing IK and linear trajectory handlers.
- 2026-07-10: `OnBnClickedBtnLinearTrajectory` already reads `m_ikX/m_ikY/m_ikZ` as the trajectory start and `m_trajEndX/Y/Z` as the end. `ExecuteLinearTrajectory` interpolates positions but currently passes a fixed pose to every IK call.
- 2026-07-10: `resource.h`, `.rc`, constructor, and DDX already define inputs for start point (`IDC_EDIT_IK_X/Y/Z`) and trajectory end/steps. No new controls are needed for start coordinate entry.
- 2026-07-10: FK constructs the tool transform along local Z. A natural next-point pose for a line is therefore a rotation whose local Z axis follows the vector from the current trajectory point to the next point.

## Bugs

- 2026-07-10: No bugs recorded yet.
- 2026-07-10: Build is blocked by `KongTan8dianji.rc(98): error RC2104: undefined keyword or key name: MS`, apparently in the resource file rather than the new C++ trajectory logic.

## Implementation

- 2026-07-10: Added `ComputePoseTowardNextPoint`; it sets local tool Z toward the next trajectory point using `Rx=0`, `Ry=atan2(horizontal, dz)`, `Rz=atan2(dy, dx)`.
- 2026-07-10: Invalid linear trajectory inputs are now rejected in the button handler before any motor reset or movement is attempted.
