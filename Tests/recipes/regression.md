# Regression behavior checks

Use the [native driver](native-validation.md) for bounded diagnostic/final runs
and authoritative host reports. This recipe describes focused foreground checks
after generation/build in an already prepared worktree. It does not provide the
workflow adapter's process ownership or edit-restoration guarantee.

[`Regression.cpp`](../src/Regression.cpp) dispatches the selectors below.
No arguments run the complete suite, including the five Editor UI fixtures.
Unknown selectors exit 2. Exceptions fail with a nonzero exit in Debug and
Release; acceptance checks remain active with `NDEBUG`.

Run from **each executable's own directory**, where runtime assets and temporary
fixture paths are resolved. Set `$prepared` to the absolute prepared worktree:

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location (Join-Path $prepared "Binaries/windows-x86_64/$configuration/Regression")
    try {
        $output = & '.\Regression.exe' camera-inverse 2>&1
        $exitCode = $LASTEXITCODE
        $output | Write-Output
        if ($exitCode -ne 0 -or $output -notcontains 'PASS: camera-inverse') {
            throw "$configuration camera-inverse failed"
        }
    } finally { Pop-Location }
}
```

Replace `camera-inverse` and its exact marker together for a focused check.
For a complete foreground run omit the argument and inspect every selector
marker below plus `PASS: editor-issue-12`, `PASS: editor-issue-15`,
`PASS: editor-issue-16`, `PASS: editor-issue-17`, `PASS: editor-issue-18`.
The driver checks the full-suite exit/pass marker and independently requires
the exact standalone-selector and UI-wrapper markers, with bounded execution
and log retention. A zero exit without required markers does not establish
completion.

| Selector (marker is `PASS: <selector>`) | Behavior / source |
| --- | --- |
| `scene-hooks` | Real component addition/removal, registry and camera viewport hooks; [entrypoint](../src/Regression.cpp) |
| `camera-inverse` | Owned inverse snapshot without changing projection; [check](../src/CameraInverseCheck.cpp), [Camera contract](../../Engine/Source/Rubber/Renderer/Camera.h) |
| `renderer-batches` | Renderer batch boundaries and real GL output; [check](../src/RendererBatchesCheck.cpp) |
| `entity-tags` | Literal long/percent/Unicode hierarchy tags and Inspector input; [check](../src/EntityTagsCheck.cpp) |
| `viewport-pixels` | Actual viewport/framebuffer/camera pixels and restoration; [check](../src/ViewportPixelsCheck.cpp) |
| `minimize-restore` | Native minimize/restore and fixed-update cadence; [check](../src/MinimizeRestoreCheck.cpp) |
| `layer-partition` | Layer partition behavior; [check](../src/LayerPartitionCheck.cpp) |
| `typed-events` | Typed event delivery; [check](../src/EventChecks.cpp) |
| `event-names` | Exact subscriber names; [check](../src/EventChecks.cpp) |
| `font-atlas` | Named atlas fonts and fallback state; [check](../src/FontAtlasCheck.cpp) |
| `atlas-placeholders` | Missing-frame slot order, handles, fallback identity and actual GPU pixels; [check](../src/AtlasPlaceholdersCheck.cpp) |
| `gl-boundaries` | Sliced strings, packed RGB/RGBA rows and GL readback; [check](../src/GLBoundariesCheck.cpp) |
| `gl-buffers` | Repeated header inclusion, buffer ownership and Debug assertion diagnostics; [check](../src/GLBuffersCheck.cpp) |

Regression creates/attaches a private desktop before GLFW/Application. Most
checks use the actual hidden OpenGL 4.5 context; use a compatible real driver
for the native plan. Private-desktop failure is fatal; do not bypass isolation.
Camera-only success does not execute the separate doctest
[UnitTests acceptance](unit-tests.md). Debug-only unset-buffer diagnostics use
scoped SEH; Release checks valid-buffer behavior without invoking that invalid
access. Existing lifetime/deferred behavior limits remain described in the
[unaltered historical contracts](../HISTORY.md).

Final acceptance still requires the full suite, all standalone selectors and
Windows wrappers in **both** configurations; focused success is diagnostic
evidence. Preserve candidate/configuration, exact command, exit/markers and raw
logs under the host report's evidence directory.
