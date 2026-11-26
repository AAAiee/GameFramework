---
name: verify-rb-editor
description: Drive the RB Editor Windows Dear ImGui scene editor the way a user does. Launch Editor.exe or Regression.exe, run doctor, exercise hierarchy, inspector, file, and viewport features, and capture evidence. Use for /verify-rb-editor, after Editor or Scene UI changes, or when proving Editor behavior on a display host with OpenGL.
---

# Verify RB Editor

RB Editor is the Windows x64 Dear ImGui scene editor in this repo (`Editor.exe`, window title `Editor`). A user docks Scene Hierarchy, Property Channel, ViewPort, and Setting, then creates entities, edits components, and saves `.rubber` files. This skill is the agent recipe for proving that behavior on a real display host.

Secondary programs exist and are not this skill's default drive. `UnitTests.exe` is headless doctest. `EngineSmoke.exe` and `EditorSmoke.exe` construct `Application` without the editor panels. Hosted `windows-latest` has no OpenGL driver. Do not treat a UnitTests pass or a CI compile as Editor proof.

Work from the GameFramework repo root. Default configuration is Debug.

## Launch

Build once, then start each unattended drive as its own `Regression.exe` process. `Regression.exe` is short-lived. It creates a private Windows desktop named `RubberRegression-<pid>` and does not switch the input desktop.

```powershell
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 build
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 doctor
```

`build` runs bundled Premake `vs2022` and MSBuild on `Editor\Editor.vcxproj` and `Tests\Regression.vcxproj`. Ready binaries are:

- `Binaries\windows-x86_64\Debug\Editor\Editor.exe`
- `Binaries\windows-x86_64\Debug\Regression\Regression.exe`

Each must sit next to `assets\shader\CommonShader.glsl` and `assets\fonts\OpenSans\static\OpenSans-Regular.ttf`. Run those executables from their own output directories so relative `assets/` paths resolve.

An unattended feature drive is ready when `doctor` prints `OK regression=` for that configuration. Each selector starts a fresh `Regression.exe`, waits for `PASS: <selector>`, and exits.

To show the live editor on the interactive desktop, copy the Editor output directory into an isolated run folder and start `Editor.exe` from there.

```powershell
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 launch-editor
```

That process is ready when all of these hold:

- The PID in `Binaries\Verify\<run-id>\instance\state.json` is alive.
- A visible window owned by that PID has title `Editor` (the GLFW window, not the console).
- `instance\editor.stdout.log` contains `Window System Init` (scene systems installed in `EditorLayer::onAttach`) or `Window Created!: Editor`.

`RB_PROFILE_ENABLED` is `0`. Profiler JSON files are not written and are not a ready signal.

Empty launch state is an empty `Scene::create()` with no entities. Sample scenes live at `assets\scene\saved\scene.rubber` and `sceneRed.rubber` and are opened through File, not loaded automatically.

Teardown is `cleanup`. It stops only the PID recorded in that run's `state.json`.

## Doctor

Run doctor before the first drive, after any failed drive, and whenever the instance looks wrong.

```powershell
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 doctor
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 doctor -RunId <run-id>
```

Without `-RunId` the check is read-only against the checkout. It confirms repo path, `git rev-parse HEAD`, MSBuild, `Editor.exe`, `Regression.exe`, and the two asset files beside each exe. With `-RunId` it also confirms the recorded PID is alive, its path equals the isolated `Editor.exe`, a visible `Editor` window exists, and `instance\editor.stdout.log` contains `Window System Init` or `Window Created!: Editor`.

Refuse to drive when doctor prints `FAIL`. Do not attach to an `Editor.exe` whose PID is absent from this run's `state.json`.

## Drive

Unattended proof uses `Regression.exe` through `rb-verify.ps1 drive`. The first-party fixtures compile production `EditorLayer`, `SceneHierachyPanel`, and `FontManager`. `EditorDriver` clicks, drags, types, and chords the labeled items at their real positions, and a Win32 session answers the native file dialogs. They run on the private desktop. ImGui does not expose UI Automation. There is no click API on standalone `Editor.exe`.

```powershell
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-hierarchy
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-inspector
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-files
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-viewport
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-exit
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-escape
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive entity-tags
```

`drive editor-ui` runs `ui-hierarchy`, `ui-inspector`, `ui-files`, and `ui-viewport` in that order, one process each. `ui-exit` and `ui-escape` close the editor, so run each as its own drive. A full pass is `drive editor-ui`, then `drive ui-exit`, then `drive ui-escape`.

`drive entity-tags` selects a hierarchy row, expands it, and types empty, percent, UTF-8, 255-byte, 256-byte, and 8192-byte tags through the production Tag input. It then deselects, reselects, deletes the row, and adds Camera and Visibility through `Add Component`. `scene-hierarchy`, `inspector`, and `entity-tags` are aliases of one Regression check. One run of any alias is enough for that check.

`rb-verify.ps1` owns selector names, timeouts, and `PASS:` markers. One Regression drive at a time. A second `drive` while `.drive.lock` holds a live PID is refused.

Read `features/` before a run and drive every entry point that feature file lists. A pass through one convenient selector does not cover a listed entry point you skipped. A feature file can name another selector for a case the drives above do not cover.

`Ctrl+N` / `Ctrl+O` / `Ctrl+S` poll `Input::isKeyPressed` on the real window. Synthetic ImGui key events do not fire those shortcuts. Escape posts a window-close event and exits the editor.

`launch-editor` is for a visible window and a screenshot. Do not send mouse coordinates into that window as proof. ImGui hit testing is not a documented driver.

## UI catalog

`Tests\src\UiCatalog.h` lists every user-facing Editor control the driver must reach. Each `kUiCatalog` row gives the id, kind, reveal gesture, and observe check. Read that table for widget names. Do not keep a second list in this file. The in-repo driver contract is `docs/testing/editor-ui-driver.md`.

Completeness has two checks.

1. Hosted. `Tests/Windows/scan-ui-catalog.ps1` compares `Editor/src` widget literals with `kUiCatalog` in both directions. A label missing from the catalog fails, and a catalog id with no source literal fails. `Orthographic` and `Perspective` come from `ImGui::Selectable(projectionTypeName[i])`, and `ItemProbe` tags `##Splitter` from the dock geometry, so those three ids skip the literal rule.
2. Display host. Run the full pass with one `-RunId` so all evidence lands in one run folder. `Tests/Windows/check-ui-coverage.ps1 <run-id>\evidence` unions the `DRIVEN: ` lines from those logs. It fails for a catalog id that is neither driven nor a window seen in the item dump. `File` / `Exit` is driven by `drive ui-exit`, so the union needs that run too.

`verified-unreachable` is not a legal status for a catalog id. A catalogued control that the driver cannot reach is a failing completeness check, not a skip. Do not substitute `Scene::createEntity` or `saveSceneTo` for a menu the catalog claims.

Hosted CI runs `scan-ui-catalog.ps1` and `UnitTests` on `windows-latest`. Live clicks run only on the display host.

## Evidence

Proof artifacts go under `Binaries\Verify\<run-id>\evidence\`. That directory is inside gitignored `Binaries/`. Cleanup deletes `instance\` and never deletes `evidence\`.

A `drive` writes one subdirectory per feature under `evidence\`, holding `stdout.log` and `stderr.log`, and appends to the shared `summary.txt` at the evidence root. Reusing a `-RunId` across a full pass keeps every drive's logs and summary lines in one run folder. `summary.txt` records run id, HEAD, selector, exit code, and every `PASS:`, `FAIL:`, `OBSERVED:`, and `DIALOG` line. Native-dialog work trees matching `issue-*-work-*` are copied next to those logs. `launch-editor` copies `editor.stdout.log` into evidence. `capture` writes `editor.png` of the GLFW window after restoring and foregrounding it. `capture` uses the screen pixels of that window rectangle, so overlapping desktop windows can appear in the PNG. Assert the title `Editor` and the panel names, not a clean desktop.

Proof standards:

- Exercise the production EditorLayer, hierarchy, inspector, dialogs, or window path named in the feature file. Do not call `SceneSerializer` or `saveSceneTo` as a stand-in for File menu or `editor-issue-15`.
- Capture the action and the resulting state. Keep the `PASS:` marker and the `OBSERVED:` lines, not only the final screenshot.
- Confirm side effects that the feature claims. Hierarchy delete must drop the registry handle. Save As must write a `.rubber` file whose Tag, Transform, Sprite color, Camera, and Visibility fields match after Open.
- Mocks are not used. Native dialogs stay real. Renderer checks go through the live OpenGL driver.
- `EditorSmoke` writes `EditorSmoke.rubber` through `SceneSerializer` with no ImGui and no dialog. That is not Editor UI proof.

## Cleanup

```powershell
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 cleanup -RunId <run-id>
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 cleanup
```

The first form stops that run's recorded PID and deletes its `instance\` directory. The second form does the same for every run under `Binaries\Verify\`. Neither form kills by process name. Neither form deletes `evidence\`. After cleanup, confirm the evidence path still exists.

`drive` already kills the `Regression.exe` it started, including on timeout. Run cleanup after a failed launch-editor attempt so the isolated Editor copy and PID do not linger.

## Helpers

All commands below are from the repo root. The script is `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1`.

- `build` generates the VS2022 projects and builds Editor plus Regression.
- `doctor` reports whether this checkout (and optional run) is worth driving.
- `drive <feature>` starts one Regression selector, waits for its `PASS:` marker, and writes evidence.
- `launch-editor` copies Editor output to `Binaries\Verify\<run-id>\instance\Editor` and starts `Editor.exe`.
- `capture -RunId <run-id>` screenshots that GLFW window to `evidence\editor.png`.
- `cleanup` stops recorded PIDs and removes instance directories.

Pass `-Configuration Release` when proving a Release build. Pass `-RunId` to reuse a run folder. Print lines `DRIVE_OK`, `LAUNCH_OK`, `CAPTURE_OK`, and `EVIDENCE=` are the machine-readable results.

`Tests\Windows\issue-12.ps1` through `issue-18.ps1` wrap the same selectors and write logs next to `Regression.exe`. Use `rb-verify.ps1` for this skill so evidence lands in `Binaries\Verify\<run-id>\evidence\`.

## Isolation

Regression windows live on a private desktop. They can run while the user has another Editor open. Do not run two `drive` invocations at once. They share one Regression output directory. `rb-verify.ps1` restores the committed `Editor/imgui.ini` before each child, so every drive starts from the docked layout. A stale minimized or floating layout breaks the next matrix run at `##Splitter`.

`launch-editor` uses a copied work directory so it does not rewrite the build tree's `imgui.ini`. Two isolated copies may run if each has its own run id. Refuse a second launch into a run whose PID is still alive.

Never drive the user's already-running Editor. That process owns the user's layout and scene files.

This host needs a real OpenGL 4.5 driver. Hosted GitHub `windows-latest` cannot run these drives.
