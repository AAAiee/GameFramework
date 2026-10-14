# First-party regression checks

`Regression` links the production Engine with C++20 and the existing Premake/MSVC
configuration. Run `Regression.exe` for all checks, or `Regression.exe scene-hooks`
for F01. Failures use exceptions and a nonzero exit status rather than `assert`,
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
Record actual commands, exit codes and observations on issue #12. Do not infer
runtime success from source inspection or project generation.

## Evidence for this iteration

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
