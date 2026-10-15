# First-party regression checks

`Regression` links the production Engine with C++20 and the existing Premake/MSVC
configuration. Run `Regression.exe` for all checks, `Regression.exe scene-hooks`
for the hook check, or `Regression.exe camera-inverse` for the inverse projection
check. `Regression.exe renderer-batches` runs the F03 renderer check.
`Regression.exe editor-issue-12` runs the genuine editor fixture. No
arguments run all four checks; the scene, renderer and editor checks share one
Application. Failures
use exceptions and a nonzero exit status rather than `assert`,
so checks remain active in Release. An unknown selector exits with status 2.

The F01 check creates one real Application and one Scene, then instantiates
addition/removal of Tag, Transform, Sprite, Camera and VisibilityControl components
through Entity. It verifies registry membership, returned references and entity
removal. Camera hooks enqueue real viewport events; two dispatch calls deliver
the queued events before scene destruction. No scene rendering or application
run loop is needed. The Application creates a real GL context and initializes
Renderer2D; its window is created hidden. A Windows desktop with a
compatible OpenGL driver is required. Scene destruction precedes Application
destruction. The existing process/renderer teardown limitations are unchanged.

## Camera inverse projection (F02 / issue #13)

`Camera` owns its projection. `setProjection` copies the supplied matrix;
`getProjection` borrows camera storage (the mutable overload permits changes).
`getInverseProjection() const` returns an owned `glm::mat4` snapshot without
modifying that storage. Callers can retain the inverse after a later projection
change or camera destruction. Supply an invertible projection; singular-matrix
validation is outside this correction.

`camera-inverse` calls the production Camera accessor using a nonidentity
perspective projection (57 degrees, 16:9, near 0.25, far 150). A compile-time
assertion requires exactly `glm::mat4` as its return type. The runtime check binds
the returned value in the caller, checks all original projection elements are
exactly unchanged, replaces the camera projection, and then multiplies the
saved original by the retained inverse. Every product element must be within
an absolute tolerance of `1e-5` of identity; NaN also fails. Exceptions keep the
check active in Release. This selector requires no Application or GL context.
The existing scene and editor selectors remain available.

After the Windows solution Rebuild commands below, run these focused checks
from each executable output directory:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location ".\Binaries\windows-x86_64\$configuration\Regression"
    try {
        foreach ($selector in @('scene-hooks', 'camera-inverse')) {
            & '.\Regression.exe' $selector
            if ($LASTEXITCODE -ne 0) { throw "$configuration $selector failed: $LASTEXITCODE" }
        }
    } finally {
        Pop-Location
    }
}
```

Each invocation must exit 0 and print `PASS: scene-hooks` or
`PASS: camera-inverse`, respectively, only after its checks pass. No arguments
must still run the entire suite, including `editor-issue-12`. Issue #13 has no
additional Windows UI script configured.

Local F02 evidence: the new `CameraInverseCheck.cpp` was compiled and run through
a temporary standalone driver against the production Camera header and the
read-only GLM snapshot in `/opt/editor-dependencies/glm`, using C++20 with GCC
`-O0 -g -DRB_DEBUG -DRB_ENABLE_ASSERTS` and `-O2 -DNDEBUG -DRELEASE`. Both runs
printed `PASS: camera-inverse` and exited 0. This checks the actual matrix oracle
in unoptimized and optimized builds; it is not a Windows/MSVC Regression run.
Windows Debug/Release builds, the full suite and both configured selectors for
this commit remain pending execution by the Windows host. The prior accepted
base's host results do not validate this new change.

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

After each observed panel dimension change, the fixture waits for two subsequent
Application frame dispatches before checking the camera against the framebuffer's
integer pixel aspect ratio. EventManager first promotes newly queued events, then
delivers them on the next dispatch; multiple fixed updates in one frame do not
advance that queue. The camera assertion still fails if WindowSystem does not
deliver the resize. The existing frame bound and script timeout also fail if the
viewport never settles.

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

## Windows attempt 1 and bounded repair

Host validation of `180f91b74fb98b2c5d35fb5f817d7e8e72c889ef` generated
the solution and completed the fresh Windows x64 Debug solution Rebuild with
exit 0 using the exact Premake and Debug MSBuild commands above. From
`C:\FinalPolishedProjects\Editor-validate-12\Binaries\windows-x86_64\Debug\Regression`,
the host ran `.\Regression.exe` with no arguments. It printed `PASS: scene-hooks`
but exited 1 in the editor fixture with
`Installed WindowSystem did not deliver the camera viewport resize`.
The log showed delivery of 1600x900 and 1600x852 camera events before failure.
Evidence is in the host's
`.sandcastle\logs\spec-11\issue-12-180f91b74fb98b2c5d35fb5f817d7e8e72c889ef-attempt-1`
directory (`build-Debug.log` and `regression-all-Debug.log`).

The fixture previously asserted immediately after an update could enqueue a new
resize, before the two required frame dispatches. This repair waits those frames
after the latest observed viewport change, preserving the framebuffer, rendering
and camera assertions and both selectors. It changes no production event or
viewport behavior. Local `git diff --check` passed; the repaired runtime and fresh
Windows Debug/Release builds remain pending host validation. Attempt 1 stopped
at the Debug suite failure, so it supplies no Release or Windows-script pass.


## Renderer batch limits (F03 / issue #14)

The Application fixture requests a hidden OpenGL 4.5 core context before window
creation, compatible with the production DSA calls and GLSL 450 shader. The F03
check requires 32 fragment texture units. It uses the initialized production
Renderer2D, real Texture2D resources and a 128 by 128 RGBA8 framebuffer; it does
not use a replacement renderer. The editor fixture still shows its own window
when selected or reached in the complete suite.

`renderer-batches` verifies 10,000 and 10,001 quads through both matrix entry
points, every colored/textured/subtexture wrapper (including rotation), and mixed
calls with either color or texture triggering the split. Every quad covers a
separate expected pixel, and the entire target is read back, including untouched
pixels. Distinct texture colors test 31 user slots, the 32nd texture, reuse of a
full slot map, white slot zero, and both a new and an existing high-slot texture
when quad and texture capacity coincide. Overlapping calls verify ordering over
both kinds of flush; an asymmetric rotated/scaled matrix verifies transforms.
Nonuniform texels and changed caller UV storage verify four-coordinate borrowing
and tinting. Empty/repeated ends and consecutive scenes (including the legacy
camera overload with a different view) check reset and camera boundaries.

A scoped observer of GLAD's `glNamedBufferData` and `glDrawElements` entry points
forwards every submission to the actual driver. It checks exact upload/draw
counts, index counts and upload bytes against the actual VAO stride: no batch
can exceed 40,000 vertices or 60,000 indices. These independent observations
complement framebuffer output and production statistics; no private renderer
state or assertion-enabled build is needed. All conditions throw on failure in
Debug and Release. GL entry points, framebuffer and viewport are restored before
the remaining suite runs.

After the existing solution generation and Debug/Release Rebuild commands, run
from each configuration's Regression output directory (with its copied assets):

```powershell
& '.\Regression.exe' renderer-batches
if ($LASTEXITCODE -ne 0) { throw 'renderer-batches failed' }
& '.\Regression.exe'
if ($LASTEXITCODE -ne 0) { throw 'complete regression suite failed' }
```

The selector prints `PASS: renderer-batches` only after all runtime checks pass
and exits 0. Existing `scene-hooks`, `camera-inverse` and `editor-issue-12`
selectors remain available. No Windows UI script is configured for this issue.

Renderer2D is process-wide and owned by Renderer/Application. Initialization,
scene submission and shutdown belong on the render thread with the compatible
context current. Pair beginScene/draws/endScene; scenes must not overlap.
endScene submits pending work, and an empty scene submits nothing. Batches hold
10,000 quads and 31 distinct user textures in addition to the renderer-owned
white slot zero. Existing textures reuse slots; a new texture or quad that cannot
fit flushes before appending it. Texture references survive until submission;
optional UV pointers borrow exactly four coordinates for the duration of the
call and copy them into the batch. Null UVs select the full texture.

Implementation verification in this Linux container is recorded below. Windows
MSVC builds and the new runtime checks remain pending host execution; the
previous issue #13 host results cover the accepted base only.

Local F03 verification: `git diff --check` passed. Premake generation with
`--file=Build.lua --os=windows vs2022` exited 0 in an isolated temporary copy
using copies of `/opt/editor-dependencies`. The generated Regression project
includes all existing fixtures plus RendererBatchesCheck, C++20, and x64 Debug
and Release configurations. No generated files or dependency changes are part
of this patch. GCC C++20 syntax-only checks of Renderer2D.cpp and
RendererBatchesCheck.cpp exited 0 using the supplied include snapshots and a
temporary forced-include adapter for existing MSVC `ui16`, `ui32`, and `ui64`
literal suffixes. Without that adapter, GCC rejects those existing suffixes.
This is supplementary syntax evidence only; neither the Windows executable nor
the GL runtime assertions have been executed here.
