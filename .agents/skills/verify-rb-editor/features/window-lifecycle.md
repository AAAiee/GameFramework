# Window lifecycle

The Editor GLFW window can be minimized and restored. While minimized, fixed 1/120s updates do not accrue a backlog. Restore resumes the cadence without replaying the minimized gap.

## Sub-features

- `window-minimize` minimizes the native window.
- `window-restore` restores it and resumes the 1/120s fixed step.
- `window-escape` closes the application when Escape is pressed.
- `window-exit` closes from `File` / `Exit`.

## How to get to it (user POV)

- Click the native minimize and restore buttons on the `Editor` window.
- Press Escape.
- Choose `File` / `Exit`.

## Driving it with rb-verify

Preconditions:

- `rb-verify.ps1 doctor` reports `OK regression=` for Debug.
- No other `drive` holds `Binaries\Verify\.drive.lock`.

- **Minimize and restore.** Drive the production window through native minimize and restore. Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive window-lifecycle`. Exit code `0`. `evidence\summary.txt` contains `PASS: editor-issue-18`, an `OBSERVED: minimized` line with `fixed updates=0`, and an `OBSERVED: restored` line.
- **File / Exit.** Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive editor-ui`. Exit code `0`. `evidence\summary.txt` contains `OBSERVED: File / Exit closed the editor`.
- **Escape.** Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive editor-ui-escape`. Exit code `0`. `evidence\summary.txt` contains `PASS: editor-ui-escape` and `OBSERVED: Escape posted WindowCloseEvent and closed the editor`.
- **Proof.** Keep `evidence\stdout.log` and `summary.txt`. A screenshot of a restored window without that marker is not enough.

## Gotchas

- `drive window-lifecycle` still covers native minimize/restore only. Escape is `drive editor-ui-escape`. File / Exit is `drive editor-ui`.
- Escape on a live `launch-editor` instance closes the process. Do not send Escape while capturing a screenshot.
- Minimized frames still poll and render ImGui. They discard lag. Restore skips the minimized delta.
- Timeout is 60 seconds. Kill only the Regression process this drive started.
