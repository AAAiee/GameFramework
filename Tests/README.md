# First-party regression checks

`Regression` links the production Engine with C++20 and the existing Premake/MSVC
configuration. Run `Regression.exe` for all checks, `Regression.exe scene-hooks`
for the hook check, or `Regression.exe camera-inverse` for the inverse projection
check. `Regression.exe renderer-batches` runs the F03 renderer check.
`Regression.exe editor-issue-12` runs the genuine editor fixture.
`Regression.exe editor-issue-15` runs the native scene dialog fixture.
`Regression.exe entity-tags` runs the F05 hierarchy fixture; `editor-issue-16`
runs the same assertions and also emits the Windows script marker. No
arguments run the complete suite; the scene, renderer and editor checks share one
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


## Scene Save As / native Windows dialogs (F04 / issue #15)

`Regression.exe editor-issue-15` uses a real Application/window and production
EditorLayer initialization, `saveScene`/`openScene`, fileDialog and SceneSerializer.
A friend fixture accesses the editor's private entry points and scene; it does
not replace the dialogs or serializer. A worker drives Win32 filename controls
and standard command buttons on that fixture's UI thread, including the actual
overwrite Yes/No prompt. It uses control IDs rather than translated labels.
No menu-click automation or application run loop is needed for these synchronous
modal entry points. The standalone selector attaches a scoped production
ImGuiLayer before attaching EditorLayer, and detaches it after the fixture is
destroyed. It checks that the context and both backend adapters exist. It requires an interactive Windows desktop and OpenGL 4.5,
just like the other Application fixtures.

The always-active assertions cover:

- Save As to a new `.rubber` filename in an existing directory, then reopen through
  EditorLayer and compare Tag, Transform, Sprite color, Camera settings and
  VisibilityControl data. A changed unsaved tag proves reopening loaded the file.
- Decline overwrite and cancel through EditorLayer, comparing exact destination
  bytes. Direct dialog calls separately verify empty paths on cancel and on
  decline followed by cancel. Accept overwrite, then reopen the changed scene.
- Save/open a filename containing Chinese, accented Latin and Greek characters,
  compare the exact returned UTF-16 paths, and verify the reopened scene data.
- Cancel Open, and submit a missing filename to Open: neither produces a path or
  creates a file. This does not test malformed-YAML recovery or transactional Open.
- Submit paths of exactly 260 UTF-16 code units to both dialogs (one beyond
  the supported 259, since the buffer must also hold the terminator). Existing parent
  directories and an existing Open destination isolate capacity from existence.
  Both must return empty, preserve saved bytes, and complete without a crash.
  The driver requires the oversized name to be entered and submitted before
  accepting `FNERR_BUFFERTOOSMALL` or a native rejection prompt followed by
  cancellation. An API initialization failure cannot pass this check.
  Extended path spelling is used solely to prepare/read this oversized fixture;
  production dialog calls receive ordinary paths. The editor's long-path support
  is unchanged. The dialog working directory must also remain unchanged.

The output working directory must be short enough for ordinary scene paths (the
fixture reports a specific failure otherwise). Scene files are retained under
`issue-15-work-<process-id>` beneath the Regression output directory for inspection.
Each dialog has a 15-second automation deadline; on failure the driver closes
only dialogs on its own UI thread. The script has a final 240-second bound and
kills only the Regression process it started. No success marker is printed before
all behavioral assertions pass. Existing selectors remain available; no arguments
run the editor loop fixture first, release its scene and shortcut subscription,
then run the dialog fixture using the initialized ImGui context and font atlas.
Closing the application loop does not prevent synchronous modal dialog calls.
This order avoids reinitializing the atlas behind FontManager's process-wide
borrowed pointers and keeps only one Scene subscribed at a time.

After the existing Premake generation and x64 Debug/Release solution Rebuild
commands above, run in **both** configurations:

```powershell
& '.\Tests\Windows\issue-15.ps1' -Configuration Debug -BinaryRoot "$PWD\Binaries\windows-x86_64\Debug"
& '.\Tests\Windows\issue-15.ps1' -Configuration Release -BinaryRoot "$PWD\Binaries\windows-x86_64\Release"
```

The script accepts the same BinaryRoot layouts as issue-12, requires exit 0 and
`PASS: editor-issue-15` in the fixture's stdout, then prints that exact marker.
It retains `issue-15.stdout.log` and `issue-15.stderr.log` in the output directory.
`OBSERVED:` lines describe completed automated runtime assertions, not manual
observations. Also run the configured `scene-hooks`, `camera-inverse` and
`renderer-batches` selectors and the complete no-argument suite in both builds.

File-dialog API contracts are documented at `PlatformUtil.h`: the fixed buffer
contains 260 **wide-character elements including the terminator**, permitting at
most 259 UTF-16 path code units. Both APIs return owning wide paths, or empty on
cancel/common-dialog error. Open requires an existing file; Save requires an
existing directory and confirms replacement. Dialog selection does not itself
write a file. Filters borrow the normal double-null-terminated Win32 list for
the modal call. Both keep the working directory and require a live Application
on its UI thread. Cancellation and API errors remain indistinguishable.

SceneSerializer's header documents the supported component fields and repeated
placeholder identity `123123141`; loading creates fresh entities. It does not
promise texture, script or runtime-state round trips. Reliable final write-error
reporting, YAML validation/recovery and preserving the active scene after failed
Open remain deferred. The runtime serialization entry points are assertion stubs.

F04 evidence (current Windows execution; earlier attempts are historical):

- **Source inspection:** both native APIs use explicit wide entry points and
  `std::size(szFile)` for `nMaxFile`; Open retains `OFN_FILEMUSTEXIST`, while Save
  uses `OFN_OVERWRITEPROMPT` with `OFN_PATHMUSTEXIST` and `OFN_NOCHANGEDIR`.
- **Windows builds/runtime:** Windows host execution on 2026-10-16 passed Debug
  and Release MSVC solution Rebuild, the full Regression suite, scene-hooks,
  camera-inverse, renderer-batches, and Tests/Windows/issue-15.ps1 in both builds.
  The validated implementation commit is a40f6aaa67e78e196d15823984db66cc2e3cb0fb;
  its report is .sandcastle/logs/spec-11/issue-15-verified-<SHA>/results.md.
  Automated observations covered new-file save/reopen, overwrite decline and
  confirmation, cancellation, Unicode, missing-file rejection, and native
  FNERR_BUFFERTOOSMALL for both over-capacity inputs. Follow-up documentation
  commits receive their own matching-SHA host reports; reports identify build
  targets and results rather than treating source inspection as runtime evidence.
- **Manual observations:** not run. The user requested unattended automation.
  The native-dialog fixture performed the behaviors above on a private desktop;
  these results are automated observations and are not relabeled as manual ones.

The following container verification and failed attempts are historical. They
are separate from the successful Windows host execution recorded above.

Local F04 verification: `git diff --check` passed. Premake generation with
`--file=Build.lua --os=windows vs2022` exited 0 in an isolated temporary copy
using the existing supplied dependency snapshots; the generated Regression
project includes EditorIssue15Fixture, C++20 and x64 Debug/Release. A GCC C++20
syntax-only check of the updated Regression.cpp exited 0 with `GLFW_INCLUDE_NONE`,
the dependency include paths and the same temporary MSVC-literal suffix adapter
used for F03. That check does not compile the Windows-only fixture or native
dialog source. Windows SDK/MSVC and PowerShell are unavailable here, so their
compilation and the script's runtime behavior remain unrun. No dependencies,
vendor code, submodule pins or generated artifacts were changed in the checkout.


## F04 Windows attempt 1 and bounded fixture repair

For `c0f477da2bb652401a58af61eb0bf549d00c9529`, the host completed solution
generation and the Windows x64 Debug solution Rebuild with exit 0. The complete
Debug regression invocation timed out after 240 seconds at ImGui's
`No current context` assertion. Application construction installs its ImGuiLayer,
but attachment occurs only in `Application::run()`. The dialog fixture called
EditorLayer attachment before that run and therefore accessed uninitialized ImGui.
The supplied logs are under
`.sandcastle/logs/spec-11/issue-15-c0f477da2bb652401a58af61eb0bf549d00c9529-attempt-1`.
That attempt supplies no Release, dialog-runtime or manual-observation pass.

The repair initializes the standalone fixture with the production ImGuiLayer and
checks the context and backend readiness using Release-active exceptions. The
complete suite instead runs the existing editor-loop check first and releases its
editor resources/subscriptions before the dialog check reuses its context. All
existing rendering, viewport, dialog and serialization assertions remain active;
the success marker still follows every dialog assertion. No Application lifecycle,
FontManager, vendor or native-dialog implementation is changed by this repair.

Repaired Windows Debug/Release builds, full-suite runs, configured selectors and
`Tests/Windows/issue-15.ps1` remain pending host execution. Manual observations
remain unrun and must be recorded separately from automated fixture output.

Local bounded-repair checks: `git diff --check` passed. GCC C++20 syntax checking
of Regression.cpp passed with the supplied dependencies, `GLFW_INCLUDE_NONE` and
the existing temporary MSVC-literal suffix adapter. A supplementary syntax check
of the exact new DialogImGuiScope class extracted to a temporary translation unit
also passed against the production ImGuiLayer and supplied ImGui headers with
that adapter. Without the adapter, existing `ui32` literals block GCC. A syntax
check of EditorIssue12Fixture.cpp with the adapter remains blocked by the existing
dependent-template syntax in FsmSystem.h and FsmPostMoving.h. These are syntax
checks only; they do not execute ImGui, the Windows fixture or its backend-readiness
assertions. No project-source list changed, so no new generation is needed for
this bounded repair.


## F04 Windows attempt 2 and diagnostic repair

For `63e95e72497700473b48f9721ddb1130d910d86f`, the supplied host logs record
successful generation and x64 Debug solution Rebuild. The complete Debug suite
printed passes for camera-inverse, scene-hooks, renderer-batches and editor-issue-12,
then exited 1 with `Production call did not show a native filename dialog`.
No `OBSERVED:` line was printed, so this attempt establishes no successful scene
save/reopen, overwrite, Unicode or capacity behavior. Release and the Windows
acceptance script were not reached. Manual observations remain unrun.

The log does not include the common-dialog error code, so the reason for the
first Save As return remains unresolved. This repair adds a flushed `DIALOG:`
line naming each production operation, action and input length. On missing UI or
API error, `DIALOG RESULT:` reports `CommDlgExtendedError` (captured on the calling
UI thread immediately after the production call), visible dialog count and
whether a filename was submitted. The fixture also checks that the Application
HWND is live and belongs to its calling thread before attaching the editor.
These diagnostics retain the failing assertion; they do not make a missing
native dialog pass.

The previous capacity exception could accept an immediate empty result before
submitting the oversized path. It now requires observed UI, exact input entry,
submission, and either `FNERR_BUFFERTOOSMALL` or a native validation prompt and
cancellation. Existing byte-preservation and empty-return assertions still apply.

Local verification for this diagnostic repair: `git diff --check` passed. This
Linux container has no Windows SDK/MSVC, Windows desktop or PowerShell; the
changed Windows-only fixture has not been compiled or executed here. Debug and
Release rebuilds, full-suite runs, selectors, acceptance scripts and manual
observations remain pending Windows host execution. The original native-return
failure is a blocker requiring that host's new diagnostic output, not a claimed
runtime fix.


## Native dialog automation and desktop isolation

Regression creates a private Windows desktop before initializing GLFW or editor
windows. It never switches the input desktop. If desktop creation or attachment
fails, it exits with an error instead of displaying dialogs on the user's desktop.
The issue-15 PowerShell entry runs a hidden console, bounds execution to 240
seconds, and kills only its own child on timeout. No manual clicks are required.

The native dialog driver supports legacy filename controls and the current
nested Edit control. Shell DirectUI confirmation/error buttons use UI Automation
Invoke patterns; title-bar buttons are excluded. Overwrite actions are checked
against actual destination bytes and reopened scene contents. Native prompts
remain real production dialogs; no dialog or serializer is mocked.

Capacity fixtures use an existing short root and nested components of at most
eight characters. This prevents Open from returning a shorter 8.3 alias instead
of exercising its 260-element output buffer. Both dialogs must observe submitted
input and a native rejection, return empty, and preserve destination bytes.

Host diagnosis: the earlier Save As failure was a filename-control discovery
failure. After repairing the driver, standalone Debug issue-15 completed all
assertions with exit 0 on 2026-10-16, including both buffer-capacity errors
(FNERR_BUFFERTOOSMALL). Full matching-commit Debug/Release validation follows in
the host report; the standalone run is not a claim that those builds passed.


## Literal entity tags (F05 / issue #16)

`entity-tags` drives the production SceneHierachyPanel with a real ImGui context,
font atlas, draw lists, keyboard/text event queue and vendored std::string adapter.
It creates a real Scene under Regression's hidden Application/OpenGL context.
A narrow friend fixture observes selection and calls the actual row renderer for
label evidence; no widget, adapter, Scene or delete operation is replaced.
The complete hierarchy/properties windows handle the interaction frames.
The context is scoped separately so the full suite's editor font atlas and
FontManager pointers survive. No backend or application lifecycle is changed.

The always-active checks click the row and expansion arrow, focus the production
Tag input, and use Ctrl+A, Backspace, UTF-8 input and Enter to rename it. The
sequence covers `%`, `%s`, `%n`, mixed percent text, Chinese/accented Latin/Greek,
255 bytes, 256 bytes, more than 8192 bytes, and empty text. Exact component bytes
must match after each edit. Real ImGui label logging checks the full text,
including the end of the long tag; the empty label must produce no text.
Submitted row IDs must remain equal across renames, and hierarchy expansion and
selection must survive. Blank-space deselection and clicking the renamed row
must work. A right click opens the production context menu and a mouse click on
Delete Entity must invalidate the registry handle and clear selection. A final
frame renders the now-empty scene. There is a 300-frame bound; failures throw
in Debug and Release. OBSERVED lines are printed only after their assertions.

Engine's first-party Premake project lists `misc/cpp/imgui_stdlib.cpp` once,
with PCH disabled for that translation unit. Editor and Regression consume that
single Engine implementation. Vendor source/project files remain unchanged.
Regenerate and Rebuild both x64 configurations with the existing commands above,
then run the full suite and all configured selectors, including `entity-tags`.
Run the required editor acceptance script in each build:

```powershell
& '.\Tests\Windows\issue-16.ps1' -Configuration Debug -BinaryRoot "$PWD\Binaries\windows-x86_64\Debug"
& '.\Tests\Windows\issue-16.ps1' -Configuration Release -BinaryRoot "$PWD\Binaries\windows-x86_64\Release"
```

The script accepts the existing BinaryRoot layouts, runs `editor-issue-16` from
the executable output folder, and retains `issue-16.stdout.log` and
`issue-16.stderr.log` there. It requires exit 0 and `PASS: editor-issue-16`, then
prints that marker. It has a 60-second timeout and closes only its own child.
Regression's private desktop initialization remains mandatory and fails closed;
this fixture uses a genuine panel test context without native dialogs or user
input. The existing scene-hooks, camera-inverse, renderer-batches and editor
selectors remain available; no arguments include all of them.

The panel header documents weak scene ownership and borrowed selection. Keep the
Scene and selected handle alive through rendering; setContext clears selection.
Tags are null-terminated UTF-8 text without embedded NUL, edited directly in the
component's owning std::string. There is no panel-imposed 255-byte limit; memory
and ImGui's supported string sizes still apply. Visible Unicode glyphs depend on
the active editor font's coverage. Font fallback/coverage and general external
entity-deletion validity are outside F05.

F05 execution evidence: Windows Debug/Release builds, Regression runtime and the
PowerShell script are pending host execution. No editor interaction or manual
observation has been executed in this Linux container. On the host, the fixture's
OBSERVED lines record completed automated panel interactions; they must not be
reported as manual observations. Earlier issue #15 passes cover the accepted
base and do not validate this new code.

Local F05 verification: `git diff --check` passed. In an isolated temporary copy
using `/opt/editor-dependencies`, Premake generation with
`--file=Build.lua --os=windows vs2022` exited 0. Generated project inspection found
one adapter compilation entry, in Engine with `PrecompiledHeader=NotUsing`, and
the EntityTagsCheck source in Regression. No generated files were added here.
A direct GCC C++20 syntax attempt hit existing MSVC integer suffixes and missing
dependent `template` keywords in dormant FSM headers. In the temporary copy only,
adding suffix operators and those two keywords allowed syntax checks of the new
fixture and changed production panel to exit 0. These accommodations are absent
from the committed sources. This is syntax/generation evidence, not an MSVC build
or a runtime observation. PowerShell and Windows/MSVC are unavailable here.

## Usable viewport pixels (F06 / issue #17)

`viewport-pixels` runs the production EditorLayer with real OpenGL attachments,
its scene/WindowSystem, the two-stage EventManager queue and production ImGui
backends. `editor-issue-17` runs the same checks and emits the Windows acceptance
marker. No arguments include these checks alongside every earlier selector.
Exceptions keep all assertions active in Debug and Release.

The friend fixture injects exact sampled panel sizes at the layout/update boundary
so OS window-coordinate rounding cannot hide fractional or negative inputs. Eight
updates at 800.5 by 600.5 must retain all framebuffer/attachment IDs and a texture
parameter sentinel (detecting recreation even if GL reuses names), publish exactly
one 800x600 event and give the camera that aspect ratio after two dispatch calls.
Zero in either axis, negative axes, negative pairs, subpixel-to-zero, NaN and
infinity must retain that target/projection without publishing events. A change
to 801.75 by 603.25 must yield 801x603 framebuffer specification, actual GL texture
storage, complete framebuffer, event and camera framing. Adding another camera
checks Scene's retained pixel dimensions. Rendering actual editor panels checks
exact restoration of a nondefault 137.25 WindowMinSize.x with docking both disabled
and enabled. The fixture warms the real OpenGL backend before editor attachment,
including in standalone selectors. After attachment adds fonts to the shared
atlas, it explicitly destroys and recreates the backend font texture and checks
that the atlas is built and the uploaded GL texture matches its dimensions.

The existing isolated native-dialog driver also exercises real New/Open at
fractional, zero, negative and subpixel-to-zero extents. A serialized camera is
opened through the production dialog and serializer each time. Both New camera
construction and Open deserialization must queue exactly the expected positive
pixels and produce a finite matching projection. Invalid panel sizes fall back
to the last usable framebuffer dimensions (811x607 in this check). Dialog timeouts,
filename entry and submission assertions remain mandatory. Files are created only
under a process-specific issue-17 directory in the executable output directory.
The private desktop is mandatory; no input desktop is switched or user action
required. These are automated observations, not manual observations.

EditorLayer's Application ordering is window update, event dispatch, zero or more
fixed scene updates, then ImGui rendering. Each update consumes the preceding
ImGui panel sample. Positive dimensions are truncated once to integer pixels
before comparison, and those pixels feed resize, event publication and scene
storage. Invalid samples retain the render target. New/Open use the same conversion
or the current usable framebuffer size before camera construction/deserialization.
Scene storage supplies later camera hooks; existing cameras follow deferred
WindowSystem delivery, respecting fixed aspect ratio. Two dispatches are needed
for a newly enqueued event. Resize events remain shared and do not identify a
scene; independent simultaneous scene viewport support is excluded.

Framebuffer bind selects the target and sets the GL viewport. Unbind selects
framebuffer 0 and leaves the viewport dimensions in place; it does not restore
a previous binding/viewport. Resize recreates attachments, invalidating borrowed
attachment IDs, even at an unchanged size if called directly. EditorLayer avoids
that call for unchanged integer pixels and reacquires the image attachment ID
on each ImGui render. The image explicitly constructs ImVec2 from panel extents.

After regenerating and rebuilding the existing x64 C++20 solution in both builds,
run all configured selectors (`scene-hooks`, `camera-inverse`, `renderer-batches`,
`entity-tags`, `viewport-pixels`), the complete no-argument suite, and:

```powershell
& '.\Tests\Windows\issue-17.ps1' -Configuration Debug -BinaryRoot "$PWD\Binaries\windows-x86_64\Debug"
& '.\Tests\Windows\issue-17.ps1' -Configuration Release -BinaryRoot "$PWD\Binaries\windows-x86_64\Release"
```

The script requires exit 0 and `PASS: editor-issue-17` after all assertions,
retains issue-17 stdout/stderr logs in the output folder, bounds execution to
240 seconds and closes only its own process. The runtime fixture prints OBSERVED
lines only after the corresponding behavioral assertions pass.

F06 runtime evidence: Windows/MSVC Debug and Release builds, Regression execution,
PowerShell execution and fractional/zero-size editor observations are pending the
Windows host. This Linux container cannot run authoritative Windows acceptance;
no runtime observation or acceptance pass is claimed for this change.

Local F06 verification: `git diff --check` passed. Premake Windows VS2022 generation
in an isolated temporary copy using the supplied dependency snapshots exited 0;
Regression includes ViewportPixelsCheck and retains C++20 Debug/Release settings.
GCC C++20 syntax checks of the changed EditorLayer and new ViewportPixelsCheck
passed with `-O0 -g -DRB_DEBUG -DRB_ENABLE_ASSERTS` and `-O2 -DNDEBUG -DRELEASE`.
The temporary copy reuses the prior MSVC integer-suffix adapter and dormant FSM
`template` keyword accommodations. Debug syntax checking additionally defines
RB_PLATFORM_WINDOWS and maps `__debugbreak` to `__builtin_trap`; it does not include
the Windows SDK. Initial Debug attempts without those platform accommodations
failed in existing Core.h. These are syntax/generation checks only. The Windows-only
native-dialog extension and PowerShell entry have not been compiled or run here.
No compatibility edits or generated projects are included in the patch.

## F06 Windows attempt 1 and backend linkage repair

The Windows host generated the solution for `904c414` successfully, but its
Debug Rebuild failed while linking Regression with LNK2005/LNK1169. Both the
Engine `ImGuiBuildcpp.cpp` wrapper and the ImGui project compiled the GLFW and
OpenGL backend implementations. The viewport fixture's direct font-texture
refresh caused the linker to extract the separate OpenGL backend object as well
as Engine's combined backend object. That attempt did not reach runtime checks
or Release validation and supplies no fractional/zero-size observations.

The obsolete first-party Engine wrapper is removed. The existing ImGui project
is now the sole compilation owner of both backends, shared by Editor and
Regression through Engine's existing dependency. Vendor implementations and
build files are unchanged. The fixture still calls the real font-texture refresh;
all behavioral assertions, selectors and private-desktop isolation are retained.

Local repair verification: VS2022 project generation in the isolated temporary
copy using the supplied dependency snapshots passed. Generated x64 Debug/Release
projects contain exactly one compilation of each backend, both in ImGui, retain
C++20, and include ViewportPixelsCheck in Regression. A separate GCC C++20 link
probe against the actual supplied ImGui core and OpenGL backend resolves both
`ImGui_ImplOpenGL3_Init` and `ImGui_ImplOpenGL3_DestroyFontsTexture` in unoptimized
Debug and optimized Release builds. The probe does not call GL functions or
exercise an editor context; it is linkage evidence only. Windows/MSVC Rebuild,
the complete Regression suite, all configured selectors and issue-17.ps1 in both
configurations remain pending host execution.

## F06 Windows attempt 2 and font atlas repair

The Windows host rebuilt Debug for `161475e` successfully. The complete suite
passed the earlier selectors through `editor-issue-12` and reached F06 camera
delivery at 800x600 and 801x603. It then stopped at ImGui's `Fonts->IsBuilt()`
assertion and exceeded the 240 second timeout. There was no F06 success marker;
Release and the issue-17 Windows script were not reached. This is incomplete
runtime evidence, not a completed fractional/zero-size acceptance run.

The fixture had destroyed the font texture after EditorLayer added fonts, relying
on the next backend NewFrame to recreate it. The pinned OpenGL backend's NewFrame
only creates device objects (including fonts) when its shader handle is absent.
In the complete suite that shader already exists. The fixture now explicitly
calls the backend's CreateFontsTexture after destruction and checks both CPU
atlas readiness and actual uploaded GL texture dimensions before rendering.
Warming the backend before attachment makes standalone selectors exercise this
same existing-shader path. Production code, vendor code, private-desktop
isolation, timeouts, selectors and all viewport/style assertions are retained.

Local repair verification: GCC C++20 syntax checks passed for the changed fixture
with Debug and Release flags in the existing temporary dependency-backed copy,
using the previously documented Linux compatibility accommodations. These are
not MSVC or GL runtime checks. Debug and Release link probes against the supplied
ImGui core/OpenGL backend also resolved Init, DestroyFontsTexture and the newly
called CreateFontsTexture and exited 0 without invoking GL. `git diff --check`
passed. Windows Debug/Release regression and script
execution, including completed fractional/zero-size observations, remain pending
the host; this container has no compatible GL/window runtime for those checks.

## F06 completed Windows acceptance and docking fixture repair

The earlier pending and failed attempts above are historical. On 2026-10-17,
the Windows host rebuilt the full solution in Debug and Release at `29938bf`.
Both configurations passed the complete Regression suite, the scene-hooks,
camera-inverse, renderer-batches, entity-tags and viewport-pixels selectors,
and Tests/Windows/issue-17.ps1, all with exit 0 and their required PASS markers.
The host report is `.sandcastle/logs/spec-11/issue-17-fixed-29938bf/results.json`
(with a companion results.md) in the Editor checkout; its commit field contains
the full validated SHA. Documentation followups receive their own matching-SHA
host reports before review.

Attempt 3 passed the full Debug suite but its standalone viewport selector
enabled docking for the first time on frame two, triggering the pinned ImGui
initialization assertion. The fixture now starts with docking enabled, matching
ImGuiLayer::onAttach, then renders with docking disabled and enabled again.
It retains the exact prior WindowMinSize.x assertion for every rendered frame.

Automated OBSERVED results in both configurations establish that a stable
800.5x600.5 panel retains the 800x600 framebuffer and attachments; zero, negative
and subpixel-to-zero extents preserve usable framebuffer and camera state;
changed pixels reach framebuffer, queued events and cameras; dockspace rendering
restores style; and real New/Open scene operations use integer pixels or the
last usable framebuffer for fractional and unusable extents. These observations
come from actual editor fixture assertions with real GL/ImGui backends on the
private regression desktop. Human manual observations were not performed.

## Minimize/restore (F07 / issue #18)

`Regression.exe minimize-restore` runs the real Application fixed-update policy
and an EditorLayer/window fixture. `editor-issue-18` runs those same checks and
also emits `PASS: editor-issue-18`; the no-argument suite includes both markers.
All existing selectors remain available. Exceptions and nonzero exits keep the
checks active in Debug and Release.

Application owns its window, timer and layer stack. The stack deletes the layer
pointers still stored at destruction. Static window/timer accessors borrow these
objects; EventManager/AssetManager accessors return shared ownership. Initial
attachment runs once in reverse stack order, putting ImGui initialization before
ordinary editor attachment. Fixed updates and ImGui callbacks traverse forward,
ordinary layers before overlays. Push transfers ownership but does not attach a
layer: late pushes require explicit attachment by the caller. Stack mutation
during traversal and restarting a closed Application are unsupported. This fix
does not redesign layer lifecycle or static-facility teardown.

GameLoopTimer measures seconds. `getElapsedTime()` measures time since the current
`startFrame()`. `waitForFrameEnd()` records that frame including its pacing wait;
`getDeltaTime()` returns the last completed sample, or the target duration before
the first completion. It is neither an average nor time since the last query.
Application clears lag on minimized frames and discards the preceding minimized
sample on restoration. Active samples continue to accumulate into 1/120-second
updates, retaining fractional active lag. Window polling, buffer swaps, queued
event dispatch and ImGui rendering retain their existing order, including while
minimized. Active stalls retain the existing catch-up policy.

The private `runFrame` seam is the frame body used by `Application::run`, and
`updateFixedLayers` is its actual fixed-update policy. A narrow friend fixture
checks half steps, three whole updates with a fractional remainder, either zero
window extent, clearing preexisting lag, repeated five-second minimized samples,
a five-second previous sample on restore, and immediate subsequent active work.
No test copy of the policy or replacement timer is used.

The same fixture explicitly attaches a production EditorLayer and drives real
Application frames, including its timer, window polling, event dispatch, fixed
updates and ImGui backends. It uses GLFW's native minimize/restore operations on
Regression's private desktop. It requires both native iconification and delivered
Application resize state, observes at least three wall-clock seconds with zero
fixed updates and continuing ImGui frames, checks zero updates on the restoration
frame, then verifies ordinary active-time update counts. Bounded transition
waits fail if native events are unavailable. The fixture removes and destroys
its own late-pushed layer afterward; the earlier completed editor fixture's
render callback is inert after its scene is released, so the full suite can
continue driving frames without changing Application lifecycle.

Run the following after the existing Windows Debug/Release solution builds:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location ".\Binaries\windows-x86_64\$configuration\Regression"
    try {
        & '.\Regression.exe' minimize-restore
        if ($LASTEXITCODE -ne 0) { throw "$configuration minimize-restore failed" }
        & '.\Regression.exe'
        if ($LASTEXITCODE -ne 0) { throw "$configuration complete suite failed" }
    } finally { Pop-Location }
    & '.\Tests\Windows\issue-18.ps1' -Configuration $configuration `
        -BinaryRoot "$PWD\Binaries\windows-x86_64\$configuration"
}
```

`Tests/Windows/issue-18.ps1` accepts the same absolute binary-root layouts as
issue-17, runs `editor-issue-18` from its executable output folder, captures
`issue-18.stdout.log`/`issue-18.stderr.log` there, and enforces a 60-second process
timeout. It closes only its own process. The marker requires exit 0 and completed
behavioral assertions. Regression fails closed if private-desktop isolation
cannot initialize and never switches the user's input desktop. Successful logs
record actual minimized seconds, ImGui frame count, restoration-frame updates,
and subsequent active seconds/update count in `OBSERVED:` lines. The script
prints these observations before its success marker for host evidence.

Local evidence: `git diff --check` passed. A Linux GCC C++20 syntax-check attempt
using the read-only dependency snapshots stopped on existing MSVC integer suffixes
and dormant FSM dependent-template syntax. No dependency or vendor implementation
was changed to bypass those constraints. This Linux environment cannot run the
Windows window/GL fixture or authoritative MSVC Debug/Release checks. Actual
multi-second minimize/restore observations and Windows results remain pending
host execution; the expected observations above are assertions, not claimed runs.

## Layer partition (F08 / issue #19)

`Regression.exe layer-partition` exercises the production `LayerStack` without
creating an Application or GL context. The no-argument suite also runs it.
Premake's existing `src/**.cpp` glob includes the check in both Debug and Release;
failures throw and exit nonzero, independent of `assert` or `NDEBUG`.
`PASS: layer-partition` is printed only after all assertions pass. Existing
selectors and private-desktop isolation remain available and unchanged.

The check crosses zero, one and three ordinary layers with zero, one and three
overlays, including empty and overlay-only stacks. Repeated missing-pointer pops
and attempts to pop every overlay as an ordinary layer must preserve the entire
pointer sequence. Ordinary insertions afterward must remain before all overlays.
Separate cases remove a sole ordinary layer or the first, middle and last of
three, verify the remaining order, repeat the now-absent pop and insert a
replacement. Destruction counters verify that a pop leaves its object alive, stack destruction deletes
only retained objects, and the caller can delete the popped object afterward.

`LayerStack` owns pushed pointers and deletes those still stored at stack
destruction. A successful `popLayer` or `popOverlay` removes stack ownership
without deleting the object; the caller takes responsibility for its lifetime.
`popLayer` searches only ordinary layers, so an absent pointer or an overlay
pointer is a no-op. Stack operations do not attach or detach objects. Attachment,
detachment and destruction behavior are otherwise unchanged.

After rebuilding the existing Windows solution in both configurations, run all
configured selectors and the complete suite from each output directory:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location ".\Binaries\windows-x86_64\$configuration\Regression"
    try {
        foreach ($selector in @('scene-hooks', 'camera-inverse', 'renderer-batches',
                'entity-tags', 'viewport-pixels', 'minimize-restore', 'layer-partition')) {
            & '.\Regression.exe' $selector
            if ($LASTEXITCODE -ne 0) { throw "$configuration $selector failed: $LASTEXITCODE" }
        }
        & '.\Regression.exe'
        if ($LASTEXITCODE -ne 0) { throw "$configuration complete suite failed: $LASTEXITCODE" }
    } finally { Pop-Location }
}
```

Every selector must emit its matching `PASS: <selector>` and exit 0. Issue #19
has no additional configured Windows UI script. Windows/MSVC builds, the complete
suite and configured selector runs remain pending execution by the Windows host;
Linux standalone checks do not establish that acceptance.

Local evidence: a temporary standalone C++20 driver compiled the new check with
production `Layer.cpp` and `LayerStack.cpp`, using GCC `-O0 -g -DRB_DEBUG
-DRB_ENABLE_ASSERTS -DRB_PLATFORM_WINDOWS` and `-O2 -DNDEBUG -DRELEASE`.
Both binaries printed `PASS: layer-partition` and exited 0. The same driver and
check linked against the original LayerStack implementation failed the order
assertion with exit 1. Compilation used the read-only spdlog dependency snapshot
and a temporary forced-include literal operator for the unchanged MSVC `0ui32`
profiler initializer; repository and vendor files were not adapted for Linux.
These runs exercise actual stack code and the regression oracle, not Windows
Regression's entry point or its UI isolation. `git diff --check` also passed.

## Typed events and exact subscriber names (F09/F10 / issue #20)

`typed-events` instantiates production `EventManager::dispatchEvent` for
`MouseReleasedEvent`. It checks pressed/released static and instance identities,
exactly one callback per immediate call, the matching return value, and handled
status for both consuming and nonconsuming callbacks (including clearing a prior
handled state). It also enqueues a local event that dies before delivery, checks
that changing its handled status after enqueue does not change the queued copy,
and verifies insertion order, promotion, delivery, and an event enqueued by a
callback. `event-names` exercises the actual EventBus and EventManager with the
seven-character `handler` view into `handler-extra` and an explicitly sized
three-character buffer without a terminator. It removes with those bounded
views, reuses removed names, and verifies that the full suffix-bearing name,
unrelated handlers, and the same name on another event type survive. It checks
registration order, consumption, and skipping already handled events. Oracles
throw in Release as well as Debug. Neither check needs an Application or GL
context; the Windows Regression entry point retains its private desktop setup.
The existing Premake source glob includes both checks; all previous selectors
and the no-argument complete suite remain available. No Windows UI script is
configured for this issue.

Event objects belong to their caller for immediate dispatch; the callback borrows
the supplied object synchronously. `dispatchEvent` uses `s_GetEventType()`, returns
whether the type matched, and on a match replaces handled status with the
callback result. It does not deliver named subscriptions or enqueue anything.
`enqueue` owns a copy, including the payload's current handled status. Named
callbacks borrow that queued copy only during delivery. Do not retain callback
event references. `dispatchAllEvents` delivers the current queue in insertion
order, then promotes the next queue and clears the completed queue. A newly
queued event is promoted on one call and delivered on the next; events enqueued
during delivery can arrive on the following call. Fixed updates do not advance
these queues unless they explicitly call dispatch.

Subscriptions own exact copies of names and callbacks in static per-event-type
storage. Names must be nonempty and unique within an event type; unsubscribe
requires an existing name. These preconditions retain the existing assertion
policy. Handlers run in registration order until one returns true; an already
handled event is skipped. Explicitly unsubscribe before captured objects die:
manager or Application destruction does not clear static subscriptions, and
`EventManager::create()` itself retains a shared singleton whose queues can
outlive an Application. Registering or unregistering during handler iteration
can invalidate the vector and is unsupported. Recursive queue dispatch and
unsynchronized concurrent access are also unsupported. This correction adds no
subscription tokens, deferred mutation, thread safety, or event subsystem
lifetime redesign.

After the existing Windows Debug/Release Rebuild commands, run both selectors
from each Regression output directory:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location ".\Binaries\windows-x86_64\$configuration\Regression"
    try {
        foreach ($selector in @('typed-events', 'event-names')) {
            & '.\Regression.exe' $selector
            if ($LASTEXITCODE -ne 0) { throw "$configuration $selector failed: $LASTEXITCODE" }
        }
    } finally { Pop-Location }
}
```

Each must exit 0 and print its `PASS: typed-events` or `PASS: event-names`
marker only after its assertions pass. Also run the complete suite with no
arguments. Authoritative Windows/MSVC Debug and Release builds and runtime
checks remain pending execution by the Windows host.

Local evidence: a temporary C++20 standalone driver linked `EventChecks.cpp`
with production `EventManager.cpp` and `Log.cpp`, using GCC `-O0 -g -DRB_DEBUG
-DRB_ENABLE_ASSERTS -DRB_PLATFORM_WINDOWS` and `-O2 -DNDEBUG -DRELEASE`.
Both binaries printed `PASS: typed-events` and `PASS: event-names` and exited 0.
Compilation reused `/opt/editor-dependencies` spdlog/GLM snapshots, the temporary
literal compatibility include described above, and a Debug-only compiler define
mapping `__debugbreak()` to `__builtin_trap()`. No production or vendor code was
adapted for Linux. These runs exercise real EventBus/EventManager behavior and
the same check functions used by Regression, but do not run its Windows entry
point, desktop isolation, or full suite. `git diff --check` passed.


## Named atlas fonts (F11 / issue #21)

`FontManager` stores the actual `ImFont*` returned by atlas insertion. Names are
copied on registration and looked up using the full bounded `string_view`.
`getFont` returns `nullptr` for an unknown name without inserting a registration.
`setDefaultFont` returns `true` after selecting that named pointer; an unknown
name returns `false` and preserves `ImGuiIO::FontDefault`.

`loadFontFromTTF` retains its `void` signature. It throws `std::runtime_error`
with the supplied path on open/read/format/insertion failure, nonpositive or
nonfinite size, or a locked atlas. Failure preserves any previous registration
under that name. Successful reloading replaces the name's pointer; earlier fonts
remain owned by the atlas. Files must be complete trusted TTF/OTF resources.
The loader reads and validates them with ImGui's existing stb_truetype header
before insertion, avoiding ImGui's Debug assertion for missing files and its
deferred parse assertion for unsupported data. ImGui copies the validated bytes
into atlas-owned storage before the loader's buffer is destroyed. Existing editor
load calls propagate these exceptions, so a failed asset load cannot silently
continue as a successful registration. No vendor implementation is edited.

All returned pointers borrow the current ImGui atlas. Load before a frame starts;
do not clear or replace the atlas, destroy its context, or use registered pointers
beyond that lifetime. The process-wide registry is for one atlas/context lifetime;
using old registrations with another context is unsupported. This correction does
not change Application teardown or add context recreation support.

`Regression.exe font-atlas` runs the actual FontManager with a real scoped ImGui
context, without an Application, window, or GL backend. It inserts default fonts
before, between, and after named loads, compares names to actual atlas pointers,
uses a sliced name with a separately registered suffix-bearing name and a
nonterminated buffer, selects both named defaults, repeats unknown/empty lookup,
and verifies unknown default selection leaves the default unchanged. Missing,
empty, and plain-text files must throw with path context without adding an atlas
font or registration. A failed replacement must preserve the previous pointer;
a name looked up while unknown must still support a later successful load.
Finally the real atlas must build and the retained fonts must be loaded. Temporary
failure fixtures are created beneath the executable working directory and removed
on exit. All assertions throw in Debug and Release. The existing Premake source
glob includes this check, and the no-argument suite runs it before creating the
Application. Its context restores the previous current context and no borrowed
pointer is used after destruction. Every previous selector and private-desktop
isolation remain available. No Windows UI script is configured for issue #21.

After the existing Windows Debug/Release rebuild commands, run all configured
selectors and the complete suite from each Regression output directory:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location ".\Binaries\windows-x86_64\$configuration\Regression"
    try {
        foreach ($selector in @('scene-hooks', 'camera-inverse', 'renderer-batches',
                'entity-tags', 'viewport-pixels', 'minimize-restore', 'layer-partition',
                'typed-events', 'event-names', 'font-atlas')) {
            & '.\Regression.exe' $selector
            if ($LASTEXITCODE -ne 0) { throw "$configuration $selector failed: $LASTEXITCODE" }
        }
        & '.\Regression.exe'
        if ($LASTEXITCODE -ne 0) { throw "$configuration complete suite failed: $LASTEXITCODE" }
    } finally { Pop-Location }
}
```

Each selector must exit 0 and print its matching `PASS: <selector>` only after
its assertions pass. Windows/MSVC Debug and Release builds, configured selectors,
and the full suite remain pending execution by the Windows host.

Local evidence: a temporary C++20 standalone driver compiled `FontAtlasCheck.cpp`
and the production `FontManager.cpp` with the real ImGui core/draw/table/widget
sources from `/opt/editor-dependencies/imGui`. GCC `-O0 -g -DRB_DEBUG
-DRB_ENABLE_ASSERTS` and `-O2 -DNDEBUG -DRELEASE` builds both printed
`PASS: font-atlas` and exited 0, using the production editor font assets.
No dependency downloads or vendor edits were needed. These runs exercise the
same check function used by Regression, but do not validate its Windows entry
point, private desktop, MSVC configuration, or the complete suite.
`git diff --check` passed.

## Animation atlas placeholder slots (F12 / issue #22)

`Regression.exe atlas-placeholders` uses the existing hidden Application and real
OpenGL 4.5 context, on Regression's private Windows desktop. It calls the actual
`AssetManager::loadAsset` / `AssetImporter` / `TextureImporter` path and then
resolves every frame through `AnimationAtlas::getTextureAtIndex`. Eight three-frame
fixtures cover missing first, middle, last, all, multiple, and no images. A narrow
friend check inspects exact slot count and handle order without adding public API.
Each case must have exactly the declared `frameCount` entries. The check verifies
registered atlas/frame metadata, every frame's intended name-derived handle,
shared fallback pointer identity across missing slots and managers, distinct
present-image imports, texture dimensions, and actual GPU pixels read back with
`glGetTextureImage`. Exceptions fail both Debug and Release. Its success marker
is printed only after all cases pass; no arguments still run the complete suite.

The fixtures write complete 2x2 RGBA PNGs with distinct solid frame colors beneath
the executable working directory and remove them on exit. RGBA rows are eight
bytes, and library paths use owned, terminated strings. They require neither the
separately ticketed F13 RGB unpack-alignment correction nor its bounded-string
boundary changes. The existing Premake source glob includes the new check.
No additional Windows UI script is configured for this issue; every previous
selector and private-desktop isolation remain available.

`AssetHandle` is a name-derived FNV-1a asset lookup key, with zero remapped to one;
it is separate from an Entity's scene/registry identity. A nonzero handle does not
prove registration or extend any lifetime. The existing duplicate-name assertion
and hash-collision limitations remain. AssetManager retains shared asset ownership
and copies metadata on registration; it does not populate `Asset::Handle` as a
separate entity identity. Atlas frames copy these lookup handles in insertion
order: file `i.png` occupies zero-based slot `i - 1`, even when absent. A missing
file registers its declared texture metadata with the existing shared fallback
before appending that handle; real images retain the normal texture importer.

AnimationAtlas and AnimationSpriteSheet borrow their AssetManager through a raw
pointer. Keep it alive through resource use and frame lookup; retaining an atlas
`Ref` alone does not retain its manager. Frame lookup requires an in-range index
and registered texture handle, and returns shared texture ownership. In the check,
atlases and ordinary textures are released before their managers, while the GL
context is live. The fallback remains a function-local, process-lived static
texture: it can outlive the context and attempt GL deletion after context teardown,
and it is unsuitable for context recreation. That existing ownership defect is
explicitly deferred. This change does not activate dormant animation systems or
redesign fallback, asset-manager, renderer, or Application lifetimes.

After the existing Windows Debug/Release Rebuild commands, run every configured
selector and the complete suite from each Regression output directory:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location ".\Binaries\windows-x86_64\$configuration\Regression"
    try {
        foreach ($selector in @('scene-hooks', 'camera-inverse', 'renderer-batches',
                'entity-tags', 'viewport-pixels', 'minimize-restore', 'layer-partition',
                'typed-events', 'event-names', 'font-atlas', 'atlas-placeholders')) {
            & '.\Regression.exe' $selector
            if ($LASTEXITCODE -ne 0) { throw "$configuration $selector failed: $LASTEXITCODE" }
        }
        & '.\Regression.exe'
        if ($LASTEXITCODE -ne 0) { throw "$configuration complete suite failed: $LASTEXITCODE" }
    } finally { Pop-Location }
}
```

Each selector must exit 0 and print its matching `PASS: <selector>` after passing.
Authoritative Windows/MSVC Debug and Release builds, importer/atlas/asset-manager
checks with the compatible GL context, and the complete suite remain pending
execution by the Windows host. No successful GL or Windows runtime result is
claimed by the Linux implementation.

Local evidence: GCC C++20 syntax checks passed for `AtlasPlaceholdersCheck.cpp`,
production `TextureImporter.cpp`, and `AnimationAtlas.cpp` with `-O0 -g -DRB_DEBUG
-DRB_ENABLE_ASSERTS` and `-O2 -DNDEBUG -DRELEASE`. A temporary driver included the
check's actual embedded PNG arrays and decoded all three using the read-only
`/opt/editor-dependencies/stb_image` implementation. Both builds verified 2x2
size, four channels, and every expected pixel, and exited 0. Compilation reused
the read-only dependency headers, temporary literal operators for the unchanged
MSVC `ui16/ui32/ui64` suffixes, and a Debug-only `__debugbreak()` mapping to
`__builtin_trap()`. No repository/vendor portability changes or downloads were
needed. These are syntax and fixture-decoding checks, not execution of the atlas
GL assertions, Windows Regression entry point, or complete suite.
`git diff --check` passed.

## Graphics string boundaries and packed rows (F13 / issue #23)

`Regression.exe gl-boundaries` runs production `GLTexture2D` and `GLShader`
against the suite's hidden OpenGL 4.5 context on its private Windows desktop.
The selector is also included in the no-argument suite. All existing selectors
remain available. Failures throw and exit nonzero in Debug and Release;
`PASS: gl-boundaries` is printed only after every assertion completes.

The fixture writes uncompressed top-origin RGB/RGBA TGA images under a temporary
subdirectory of the executable working directory and removes them on exit.
Distinct bytes across three rows test widths 1, 3 and 5 at unpack alignments
1, 4 and 8. Both file construction and full-image `setData` uploads must preserve
the prior alignment and match every expected byte on actual GL readback. A
size-only RGBA texture is checked too. Readback independently sets pack alignment
to 1, clears row lengths/skips, and unbinds pack/unpack buffers; saved GL state is
restored afterward. Top-origin disk rows must become bottom-first texture rows;
`setData` supplies bottom-first rows directly. The fixture passes exact pixel
byte counts, with extra backing allocation for replacement data so an erroneous
strided upload cannot read beyond that allocation.

Texture and shader file paths have nonempty unused suffix bytes beyond the view.
A linked shader exposes both `u_Value` and `u_ValueSuffix`; sliced uniform updates
must change only `u_Value`, including a repeated lookup with a different backing
suffix to exercise the cache. Program link status, independent uniform locations,
GL errors, dimensions and pixel values are checked. This uses valid shader inputs;
shader failure-path redesign remains excluded.

Textures own GL resources; `Ref` shares that ownership, and final release requires
a current compatible context. SubTexture2D retains a shared sheet reference and
owns its four UVs; `getTexCoord()` borrows exactly four values in bottom-left,
bottom-right, top-right, top-left order until SubTexture2D destruction. Copy the
values to retain them. File paths are copied, decoded RGB/RGBA pixels are freed
after upload, and `setData` borrows `width * height * channels` bytes only for the
call (3 bytes per RGB pixel, 4 per RGBA pixel; size-created textures use RGBA).
Uploads preserve unpack alignment; other unpack state must specify ordinary
client memory with no row length, skips or unpack buffer. See the corresponding
public API headers for the contracts.

From each configuration's Regression output directory, run:

```powershell
& '.\Regression.exe' gl-boundaries
if ($LASTEXITCODE -ne 0) { throw "gl-boundaries failed: $LASTEXITCODE" }
```

Run this after both Debug and Release Rebuilds described above, and run the
no-argument suite in both builds. No additional Windows UI script is configured
for issue #23. Linux source/compile checks do not establish runtime GL behavior:
Windows/MSVC Debug and Release hidden-context execution remains pending the host.

Local F13 evidence: `git diff --check` passed. GCC C++20 syntax checking of
`GLTexture2D.cpp` passed using the read-only dependency snapshots and a temporary
preinclude for the existing MSVC integer-literal suffixes. An extracted copy of
the fixture's pixel/TGA writer functions was compiled with C++20 at `-O0 -g` and
`-O2 -DNDEBUG`; both decoded all six width/channel combinations through the
supplied stb implementation and verified every channel and the vertical flip.
This validates the fixture data only, not GPU uploads. A broader local syntax
check reached the existing `GLShader.h` constexpr upload helpers, which GCC
rejects because they call GL function pointers. Those pre-existing declarations
and the C++20/Premake/MSVC configuration were preserved. This container has no
OpenGL runtime/context runner; no GL execution or Windows build is claimed.
