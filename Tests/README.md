# First-party regression checks

`Regression` links the production Engine with C++20 and the existing Premake/MSVC
configuration. Run `Regression.exe` for all checks, or `Regression.exe scene-hooks`
for the hook check. `Regression.exe editor-issue-12` runs the genuine editor
fixture. No arguments run both checks in sequence using one Application. Failures
use exceptions and a nonzero exit status rather than `assert`,
so checks remain active in Release. An unknown selector exits with status 2.

The F01 check creates one real Application and one Scene, then instantiates
addition/removal of Tag, Transform, Sprite, Camera and VisibilityControl components
through Entity. It verifies registry membership, returned references and entity
removal. Camera hooks enqueue real viewport events; two dispatch calls deliver
the queued events before scene destruction. No scene rendering or application
run loop is needed. The Application creates a real GL context and initializes
Renderer2D; its window is hidden after construction. A Windows desktop with a
compatible OpenGL driver is required. Scene destruction precedes Application
destruction. The existing process/renderer teardown limitations are unchanged.

## Editor runtime fixture

The `editor-issue-12` selector compiles the production EditorLayer, hierarchy panel
and FontManager into Regression. It runs Application's real window/event/ImGui
loop and EditorLayer's actual attachment, scene update, framebuffer and panel
rendering paths. A narrow friend declaration exposes the active scene to the
fixture without adding a public editor API. It creates a tagged entity with
Transform, Sprite, Camera and VisibilityControl, waits for real panel frames and
fixed updates, checks a rendered quad and matching framebuffer/camera viewport,
then edits the tag, removes all five components and deletes the entity. All
oracles throw in Release too. Closing before completion fails. This F01 fixture
requires a Windows desktop and OpenGL driver; it does not cover dialogs or the
later editor defect tickets. It is a genuine editor fixture, not automation of
menu clicks in Editor.exe.

The host check accepts the configuration output root (containing `Regression/`),
its parent (containing `Debug/` and `Release/`), or `Binaries/`:

```powershell
& '.\Tests\Windows\issue-12.ps1' -Configuration Debug -BinaryRoot "$PWD\Binaries\windows-x86_64\Debug"
& '.\Tests\Windows\issue-12.ps1' -Configuration Release -BinaryRoot "$PWD\Binaries\windows-x86_64\Release"
```

Premake copies the production editor assets and `imgui.ini` layout to Regression.
The script runs from the executable output directory, where assets and working
files reside. It captures stdout/stderr there, enforces a 60 second timeout,
checks exit 0 and the fixture's completed-assertion marker, and closes only the
process it started. Successful output is `PASS: editor-issue-12`.

## Windows validation commands

From the repository root in PowerShell, initialize missing dependencies first:

```powershell
git submodule update --init --recursive
```

If the solution is absent or stale, run the setup batch from its own directory
(it uses `pushd ..`), then regenerate after adding this target:

```powershell
Push-Location Scripts
cmd /c Setup-Windows.bat
Pop-Location
```

The equivalent noninteractive generation command is:

```powershell
& '.\Vendor\Binaries\Premake\Windows\premake5.exe' --file=Build.lua vs2022
```

Build the entire generated solution, including Engine, Editor and Regression,
using Visual Studio 2022 MSBuild. Adjust the installation path for your edition;
Visual Studio 2022 or JetBrains Rider can drive the same x64 MSVC solution builds.

```powershell
$msbuild = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
& $msbuild 'Rubber Engine.sln' /t:Rebuild /m /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo
& $msbuild 'Rubber Engine.sln' /t:Rebuild /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Run from each executable's directory so the copied `assets/` shader resolves:

```powershell
Push-Location '.\Binaries\windows-x86_64\Debug\Regression'
& '.\Regression.exe' scene-hooks
$debugResult = $LASTEXITCODE
Pop-Location
if ($debugResult -ne 0) { throw "Debug regression failed: $debugResult" }

Push-Location '.\Binaries\windows-x86_64\Release\Regression'
& '.\Regression.exe' scene-hooks
$releaseResult = $LASTEXITCODE
Pop-Location
if ($releaseResult -ne 0) { throw "Release regression failed: $releaseResult" }
```

Expected result in both configurations: `PASS: scene-hooks`, exit 0. Also launch
the Editor from its output directory in both configurations; create an entity,
add/remove Transform, Sprite, Camera and Visibility controls through the existing
UI where offered, and verify ordinary scene editing. Tag add/remove is exercised
by the regression executable because the editor expects every entity to have a tag.
The Windows host records actual commands, exit codes and observations on
issue #12. Do not infer
runtime success from source inspection or project generation.

## Evidence from the initial repair

The Linux checkout initially contained both unconditional `static_assert(false)`
primary hooks and no regression target. The regression was written before the
repair; Windows compilation/runtime of that red state was unavailable. The
implementation now uses type-dependent assertions and preserves all supported
explicit hook specializations. No blanket successful primary hook was added.

Local verification: `git diff --check` passed. The command
`Vendor/Binaries/Premake/Linux/premake5 --file=Build.lua --os=windows vs2022`
exited 1 because `Engine/RBVendor/glfw/Build-GLFW.lua` was absent. All five
Git submodules were uninitialized; `git submodule update --init --recursive`
stalled on the initial GLFW clone and was stopped. Project generation is therefore
unverified, and dependency initialization is required on the Windows host.

Windows x64 Debug/Release compilation, linking, regression runs and editor
observations have **not been run** in this sandbox. The earlier Debug-only
experiment reported in parent issue #11 is historical evidence, not validation
of this combined change. Windows validation remains a required human gate.

## Existing scene contracts and limits

Scene owns its registry and system objects. Construction installs WindowSystem
then RendererSystem and initializes them immediately; updates and shutdown use
that insertion order. SystemBase borrows the Scene and registry; initialization
must precede updates, whose delta is in seconds. Input, animation, movement,
collision, scripts, FSM and the separate camera system are not installed by
Scene::systemsInit. RendererSystem initialization requires the live Application
window/context; constructing a Scene alone is insufficient.

Entity borrows its Scene and handle. Component references borrow registry storage;
removal, storage relocation or Scene destruction can invalidate them. Entity's
boolean conversion is not a full validity check. Hooks run before registry
insertion/removal. Their primary definitions are in Scene.cpp: unsupported types
fail dependently when instantiated there, but callers in another translation
unit may receive an unresolved symbol because those definitions are not visible.
This change does not redesign those diagnostics or component hooks.

WindowSystem instances share the `Viewport Resize` subscription name and events
carry no scene identity. Multiple live scenes cannot rely on independent viewport
subscriptions. Keep one Scene alive at a time in these fixtures. Independent
scene viewports, entity validity/foreign-scene removal, native script ownership,
dormant-system defects and application/static-facility teardown remain excluded
by the [parent specification](https://github.com/AAAiee/GameFramework/issues/11#issuecomment-6151488433).

## Assigned-task follow-up evidence

The assigned branch was clean at inspection and already contained the reviewed
F01 repair (20d4d0e) and API documentation review (73f464e). The dependent
assertions and supported specializations have been retained. This follow-up adds
the required Windows script and real editor fixture, strengthens camera-hook
runtime checking, and keeps the `scene-hooks` selector available. Local inputs
were reused from `/opt/editor-dependencies` and `/opt/editor-local-docs`;
dependency downloads and vendor edits are unnecessary.

Windows x64 Debug/Release Rebuild and both runtime selectors are **pending host
execution**. No successful Windows build or runtime result is claimed by this
Linux implementation. Use the exact Rebuild and script commands above for that
gate; the earlier Debug-only experiment remains insufficient.

Follow-up local checks: `git diff --check` passed. In an isolated temporary copy
with the supplied dependency snapshots, running
`Vendor/Binaries/Premake/Linux/premake5 --file=Build.lua --os=windows vs2022`
exited 0. The generated Regression project contains both x64 configurations,
C++20, the fixture and all three production editor source files. Generated files
remain outside this checkout. This validates generation only, not MSVC builds.
A Linux `g++ -std=c++20 -fsyntax-only` attempt with the supplied dependency include
paths failed on existing MSVC numeric literal suffixes and dormant FSM template
syntax. No portability or dormant-system corrections were made. PowerShell is
not installed here, so the Windows script also awaits host execution.
