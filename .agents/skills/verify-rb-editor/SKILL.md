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

An unattended feature drive is ready when `doctor` prints `OK regression=` for that configuration. Each `drive` invocation starts a fresh `Regression.exe`, waits for `PASS: <selector>`, and exits.

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

Unattended proof uses `Regression.exe` through `rb-verify.ps1 drive`. The first-party fixtures compile production `EditorLayer`, `SceneHierachyPanel`, and `FontManager`, then inject real ImGui mouse, key, and text events or drive the native Win32 file dialogs. They run on the private desktop. They are the existing driver. ImGui does not expose UI Automation. There is no click API on standalone `Editor.exe`.

```powershell
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive scene-hierarchy
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive inspector
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive scene-files
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive viewport
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive window-lifecycle
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive editor-ui
pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive editor-ui-escape
```

`rb-verify.ps1` owns selector names, timeouts, and `PASS:` markers. One Regression drive at a time. A second `drive` while `.drive.lock` holds a live PID is refused.

Read `features/` before a run and drive every entry point that feature file lists. A pass through one convenient selector does not cover a listed entry point you skipped. If a listed entry point cannot be reached, the editor-ui driver is the missing piece. Do not report `verified-unreachable` for a catalogued Editor control. Do not substitute `Scene::createEntity` or `saveSceneTo` for a menu the map still claims. `drive editor-ui` clicks every catalogued control: File New/Open/Save As/Exit, Create Entity, Delete Entity, Transform X/Y/Z, Sprite Color Picker, Remove Component, Camera Primary/IsFixedRatio/Projection/Size/FOVY, Visibility isVisible, the ViewPort splitter, and Ctrl+N/O/S. `drive editor-ui-escape` posts Escape.

Stable ImGui names in production source:

- Windows: `DockSpace Demo`, `Scene Hierarchy`, `Property Channel`, `ViewPort`, `Setting`
- File menu: `New` (`Ctrl+N`), `Open...` (`Ctrl+O`), `Save As...`, `Exit`
- Hierarchy: `Create Entity` (right-click blank space), `Delete Entity` (right-click a row)
- Inspector: `Tag` header, input `##Tag`, button `Add Component`, popup items `Camera`, `Sprite Renderer`, `Transform`, `Visibility`, `Remove Component`
- Camera: checkboxes `Primary`, `IsFixedRatio`; combo `Projection` with `Orthographic` and `Perspective`; drags `Size`, `NearClip`, `FarClip`, `FOVY`
- Visibility checkbox: `isVisible`
- Sprite: `Color Picker`
- Stats in `Setting`: `DrawCount`, `QuadCount`, `IndexCount`, `VertexCount`, `Frame Time`, `Current FPS`

`Ctrl+N` / `Ctrl+O` / `Ctrl+S` poll `Input::isKeyPressed` on the real window. Synthetic ImGui key events do not fire those shortcuts. Escape posts a window-close event and exits the editor.

`launch-editor` is for a visible window and a screenshot. Do not send mouse coordinates into that window as proof. ImGui hit testing is not a documented driver.

## Evidence

Proof artifacts go under `Binaries\Verify\<run-id>\evidence\`. That directory is inside gitignored `Binaries/`. Cleanup deletes `instance\` and never deletes `evidence\`.

A `drive` writes `stdout.log`, `stderr.log`, and `summary.txt` there. `summary.txt` records run id, HEAD, selector, exit code, and every `PASS:`, `FAIL:`, `OBSERVED:`, and `DIALOG` line. Native-dialog work trees matching `issue-*-work-*` are copied next to those logs. `launch-editor` copies `editor.stdout.log` into evidence. `capture` writes `editor.png` of the GLFW window after restoring and foregrounding it. `capture` uses the screen pixels of that window rectangle, so overlapping desktop windows can appear in the PNG. Assert the title `Editor` and the panel names, not a clean desktop.

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

Regression windows live on a private desktop. They can run while the user has another Editor open. Do not run two `drive` invocations at once. They share one Regression output directory (`imgui.ini`, `issue-*-work-*`).

`launch-editor` uses a copied work directory so it does not rewrite the build tree's `imgui.ini`. Two isolated copies may run if each has its own run id. Refuse a second launch into a run whose PID is still alive.

Never drive the user's already-running Editor. That process owns the user's layout and scene files.

This host needs a real OpenGL 4.5 driver. Hosted GitHub `windows-latest` cannot run these drives.
