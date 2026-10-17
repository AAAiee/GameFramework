# Combined validation evidence for specification #11 / issue #25

## Provenance and status

The supplied Windows record `issue-24-e3bf3fff0f651b072b1357f234b853b3e2a3eea6-attempt-1` validates accepted base **e3bf3fff0f651b072b1357f234b853b3e2a3eea6**, not the later #25 documentation commit. The input files are available to this task under `/opt/editor-validation/`; raw logs and generated artifacts are not repository deliverables. `results.json` reports passed, all 31 steps exit 0, no timeouts, markers present. This publication cross-checked that summary against every focused selector log and all full-suite markers and observations. It does not infer success from the earlier draft audit.

| Supplied step | Debug | Release | Scope |
| --- | --- | --- | --- |
| Premake generation | Exit 0 (one shared generation) | Same projects | Existing Windows VS2022 configuration |
| Solution Rebuild | Exit 0 | Exit 0 | Final accepted-base Engine, Editor and Regression together |
| Regression with no arguments | Exit 0, all markers | Exit 0, all markers | Complete suite including genuine editor fixtures and native dialogs |
| Each of the 13 configured selectors | Exit 0, matching PASS | Exit 0, matching PASS | All focused selectors listed below |
| Five separate issue scripts | Unrun in this #24 record | Unrun in this #24 record | #24 plan has `windowsChecks: []`; fixture passes inside the full suite do not establish standalone script execution |
| Human interactive Editor session | No supplied observation | No supplied observation | Automated fixture evidence below is narrower |
| #25 final commit validation | Pending Windows host | Pending Windows host | Linux cannot execute authoritative MSVC/Win32/GL/ImGui acceptance |

## Exact supplied invocation

The `results.md` records cwd `C:\FinalPolishedProjects\Editor-validate-12` for generation and Rebuild. Commands (PowerShell spelling of the recorded executable and argument arrays):

```powershell
Set-Location 'C:\FinalPolishedProjects\Editor-validate-12'
& '.\Vendor\Binaries\Premake\Windows\premake5.exe' --file=Build.lua vs2022
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Rubber Engine.sln' /t:Rebuild /m /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'Rubber Engine.sln' /t:Rebuild /m /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

Both configurations used executable/cwd `Binaries\windows-x86_64\<Configuration>\Regression\Regression.exe` / its containing directory, first with no arguments and then each selector separately. This expands the exact recorded runner commands for both configurations:

```powershell
$selectors = @('scene-hooks', 'camera-inverse', 'renderer-batches', 'entity-tags',
    'viewport-pixels', 'minimize-restore', 'layer-partition', 'typed-events',
    'event-names', 'font-atlas', 'atlas-placeholders', 'gl-boundaries', 'gl-buffers')
foreach ($configuration in @('Debug', 'Release')) {
    Push-Location "C:\FinalPolishedProjects\Editor-validate-12\Binaries\windows-x86_64\$configuration\Regression"
    try {
        & '.\Regression.exe'
        if ($LASTEXITCODE -ne 0) { throw "Full suite failed: $configuration" }
        foreach ($selector in $selectors) {
            & '.\Regression.exe' $selector
            if ($LASTEXITCODE -ne 0) { throw "$configuration $selector failed" }
        }
    } finally { Pop-Location }
}
```

The host bounded each process and checked its marker as well as exit code. [Tests/README.md](../../Tests/README.md#combined-validation-for-issue-25) provides the portable final-commit recipe including the five scripts, working directories and expected markers.

## Supplied editor observations

These are **automated runtime observations** from `regression-all-Debug.log` and `regression-all-Release.log` at the accepted base. Inspection of fixtures confirms they use production EditorLayer/panels, actual GL/ImGui and native dialogs, with always-active exception oracles; no Release-disabled assertion supplies the behavioral pass. Regression creates a private desktop before UI and fails closed if isolation cannot initialize. The desktop is never switched to the user's input desktop. These fixtures do not certify arbitrary interactive workflows, glyph coverage, or excluded defects.

| Observation | Recorded result in both builds | Evidence scope |
| --- | --- | --- |
| New scene / Save As / reopen | New destination saved and supported component values reopened | `editor-issue-15`, production file operations and native dialog automation |
| Unicode Save As / Open | Path and scene preserved; missing Open rejected | Same fixture; no general Unicode font/path API guarantee |
| Cancel / overwrite | Prompt appeared; decline/cancel preserved bytes; confirmation replaced scene | Same fixture; over-capacity paths also rejected without writing/crashing |
| Fractional viewport | Stable 800.5 × 600.5 panel retained 800 × 600 target and attachment IDs | `viewport-pixels` / `editor-issue-17` |
| Zero / negative / subpixel viewport | Usable framebuffer and camera retained; New/Open used pixels or last usable dimensions | Same fixture; changed pixels reached target, events and camera, style minimum restored |
| Long / percent tags | Empty, percent, UTF-8, 255/256 and 8192+ byte input retained; literal labels and row IDs checked | `entity-tags` / `editor-issue-16`, actual hierarchy ImGui input |
| Selection / deletion | Rename retained expansion/selection; blank deselect, reselect and Delete Entity menu passed | Same fixture |
| Minimize / restore | Debug: 3.00098 s minimized, 0 fixed updates, 180 ImGui frames; Release: 3.00031 s, 0, 180 | `minimize-restore` / `editor-issue-18`, real Win32 minimize/restore |
| Restore pacing | First restore frame 0 updates; next 0.250037 s Debug / 0.250008 s Release gave 30 updates at 1/120 s | Same fixture, exercises real Application frame policy |
| Ordinary scene editing | Production panels rendered tagged Transform/Sprite/Camera/Visibility entity, one quad, matching camera/target; edited tag, removed components and entity | `editor-issue-12`; component edits use the fixture's real scene API, not all Inspector menu clicks |

No human manual observations or final #25 script runs are supplied. They remain explicitly unrun here; the Windows host executes the configured acceptance and can append evidence after review. Do not mark a manual gate passed from screenshots in the READMEs or a source search.

## Focused coverage and limitations

F01–F14 map to selectors in [the audit inventory](../codebase-improvement-plan.md). Renderer batch tests read actual rendered pixels, texture checks read GL storage, hierarchy tests use actual ImGui; native dialog automation is bounded. Scene hooks retain unsupported-type diagnostics limitations. Regressions do not authorize enabling dormant systems, simultaneous scenes, transactional deserialization, generalized Release failure handling, or teardown redesign. [The audit exclusions](../codebase-improvement-plan.md#excluded-findings-retained-from-the-audit) remain visible after these passes.

## Input fingerprints

SHA-256 identifies the supplied evidence files used by this publication; this is provenance, not a replacement for logs or a future run:

| File in the supplied #24 record | SHA-256 |
| --- | --- |
| `results.json` | `c88bced4e32380d665a01535cbbf3c28a0b843ad939427348ac75ff086672ced` |
| `results.md` | `3dbde056991a88b1e4bef22072c90c1105dbfdd70b7b87c09e34ffa8e7df6f8a` |
| `plan.json` | `559d256b1f90dc5bd11da63a9d552bfaf2471cb8454d9ecdb6d5a3e124eba7e5` |
| `generate.log` | `0098cca8e57e0b3dd77d91ea3ce067128d761b594e9b6a21167f54000377add3` |
| `build-Debug.log` | `0f88393c773ca2a7c044722258d7931c30ad3e5c3040a363351ff17217767667` |
| `build-Release.log` | `ac1ae34f2cf9725797c0acc4c87622dcdbb89df2a622fa519ae2e6199d2ace6f` |
| `regression-all-Debug.log` | `4c9effcb9ab0b9646b84a890987d46efecb39aed134f9edf81864f70e8855f2a` |
| `regression-all-Release.log` | `81919d79238ecda95714f8b8d76157fad2ff96980ea5b113c171e10fe237804b` |

## Local #25 checks

Source and documentation inspection only: reviewed all listed contract seams and the implementations they describe, reconciled both local drafts, verified local Markdown targets, and ran `git diff --check`. A token comparison after stripping comments/whitespace confirmed every changed C++ file retains identical non-comment tokens to the accepted base. No new runtime test was added for a comments-only patch; all existing selectors/scripts are preserved. No first-party behavior, source inclusion, vendor implementation, public name, C++20/Premake/MSVC configuration, generated output, submodule pin or host scheduling file was changed. Windows generation/build/runtime is unrun for this patch; dependencies were reused read-only, with no downloads.
