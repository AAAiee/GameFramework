# Scene files

File New, Open, and Save As load and store `.rubber` scenes. A user creates a new empty scene, saves through the native Save As dialog, reopens the file, cancels, declines or confirms overwrite, and uses Unicode names.

## Sub-features

- `files-new` replaces the active scene with an empty scene (`File` / `New` or `Ctrl+N`).
- `files-save-as` writes a new `.rubber` file through the native Save As dialog (`File` / `Save As...` or `Ctrl+S`).
- `files-open` loads an existing `.rubber` file through the native Open dialog (`File` / `Open...` or `Ctrl+O`).
- `files-cancel` returns an empty path on cancel and does not write a file.
- `files-overwrite` declines or confirms the native overwrite prompt and matches destination bytes.
- `files-unicode` round-trips a filename with Chinese, accented Latin, and Greek characters.
- `files-capacity` rejects a 260 UTF-16-code-unit path without crashing.

## How to get to it (user POV)

- Choose `File` / `New`, or press `Ctrl+N`.
- Choose `File` / `Open...`, or press `Ctrl+O`.
- Choose `File` / `Save As...`, or press `Ctrl+S`.
- Confirm, decline, or cancel the native Windows dialogs that those items open.

## Driving it with rb-verify

Preconditions:

- `rb-verify.ps1 doctor` reports `OK regression=` for Debug.
- No other `drive` holds `Binaries\Verify\.drive.lock`.
- The Regression output directory path is short enough for ordinary scene paths. The fixture fails with a specific message if it is not.

- **File menu New / Open / Save As and Ctrl+N / Ctrl+O / Ctrl+S.** Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive editor-ui`. Exit code `0`. `evidence\summary.txt` contains `OBSERVED: File / New cleared the scene`, `OBSERVED: File / Open... opened a native dialog`, `OBSERVED: File / Save As... opened a native dialog`, `OBSERVED: Ctrl+N overlay cleared the scene through shortcutCallBack`, `OBSERVED: Ctrl+O overlay opened a native dialog`, and `OBSERVED: Ctrl+S overlay opened a native dialog`.
- **Save, reopen, cancel, overwrite, Unicode, capacity.** Drive production `EditorLayer::saveScene` / `openScene` and the native dialogs. Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive scene-files`. Exit code `0`. `evidence\summary.txt` contains `PASS: editor-issue-15`.
- **Persisted scene.** Copy any `issue-15-work-*` directory that `drive` placed under `evidence\`. Open the saved `.rubber` text. Tag, Transform, Sprite color, Camera, and VisibilityControl fields match the values asserted in `stdout.log` `OBSERVED:` lines.
- **Proof.** Keep `evidence\stdout.log`, `stderr.log`, `summary.txt`, and the copied work tree. Dialog `OBSERVED:` lines are automated native-dialog results. They are not a manual File-menu click.

## Gotchas

- `drive scene-files` still calls `saveScene` / `openScene` for dialog submit, overwrite, Unicode, and capacity. Literal menu-bar clicks are `drive editor-ui`. Do not replace either drive with `EditorSmoke` or `saveSceneTo`.
- `Ctrl+N`, `Ctrl+O`, and `Ctrl+S` read real GLFW key state. Synthetic ImGui key events do not fire them.
- Open destroys the active scene before deserialize and ignores the boolean result. A missing or malformed file is not a recovery test.
- Serialized entity id is the placeholder `123123141`. Do not assert stable entity identity across save/load.
- Serialization stores Tag, Transform, Camera, sprite color, and Visibility only. Texture paths and scripts are not in the file.
- Dialogs stay on Regression's private desktop. Do not look for them on the user's desktop.
- Timeout is 240 seconds. Cleanup must still run if the drive times out.
