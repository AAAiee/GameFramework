# Combined validation evidence for specification #11 / issue #25

## Provenance and status

The supplied Windows record `issue-25-807ecc77d666f75e3e704d114f731db9a3a99443-attempt-1` validates **807ecc77d666f75e3e704d114f731db9a3a99443**, the first #25 publication commit on accepted base **e3bf3fff0f651b072b1357f234b853b3e2a3eea6**. The input files are available under `/opt/editor-validation/`; raw logs and generated artifacts are not repository deliverables. `results.json` reports passed: all 41 steps exit 0, no timeouts, markers present. This repair cross-checked every focused and standalone-script log and all full-suite markers. It supersedes the earlier #24 record for combined-validation status; no pass is inferred from draft documentation.

This repair adds Inspector input coverage to `entity-tags` / `editor-issue-16`. That **new coverage has not run on Windows**; the host must validate the repair commit. The recorded passes below apply to the exact preceding SHA, not to this repair. Human interactive ordinary editing remains unrun.

| Supplied step | Debug | Release | Scope |
| --- | --- | --- | --- |
| Premake generation | Exit 0 (one shared generation) | Same projects | Existing Windows VS2022 configuration |
| Solution Rebuild | Exit 0 | Exit 0 | Engine, Editor and Regression together at the recorded #25 SHA |
| Regression with no arguments | Exit 0, all markers | Exit 0, all markers | Complete suite including genuine editor fixtures and native dialogs |
| Each of the 13 configured selectors | Exit 0, matching PASS | Exit 0, matching PASS | All focused selectors listed below |
| Five separate issue scripts | Exit 0, matching PASS | Exit 0, matching PASS | `issue-12.ps1`, `issue-15.ps1`, `issue-16.ps1`, `issue-17.ps1`, `issue-18.ps1`, each launched separately |
| Human interactive Editor session | No supplied observation | No supplied observation | Automated fixture evidence below is narrower |
| This repair commit validation | Pending Windows host | Pending Windows host | Includes the added Inspector-input checks; Linux cannot execute authoritative MSVC/Win32/GL/ImGui acceptance |

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

The same record ran all five scripts in each configuration from the repository root, using this exact PowerShell executable and expanded argument values:

```powershell
Set-Location 'C:\FinalPolishedProjects\Editor-validate-12'
$pwsh = 'C:\Program Files\WindowsApps\Microsoft.PowerShell_7.6.6.0_x64__8wekyb3d8bbwe\pwsh.exe'
foreach ($configuration in @('Debug', 'Release')) {
    foreach ($issue in @(12, 15, 16, 17, 18)) {
        & $pwsh -NoProfile -File "C:\FinalPolishedProjects\Editor-validate-12\Tests\Windows\issue-$issue.ps1" -Configuration $configuration -BinaryRoot "C:\FinalPolishedProjects\Editor-validate-12\Binaries\windows-x86_64\$configuration"
        if ($LASTEXITCODE -ne 0) { throw "$configuration issue-$issue failed" }
    }
}
```

Each `windows-issue-<issue>-<configuration>.log` contains its corresponding `PASS: editor-issue-<issue>` marker. The host bounded each process and checked its marker as well as exit code. [Tests/README.md](../../Tests/README.md#combined-validation-for-issue-25) provides the portable final-commit recipe including the five scripts, working directories and expected markers.

## Supplied editor observations

These are **automated runtime observations** from `regression-all-Debug.log` and `regression-all-Release.log` at the recorded #25 SHA. Inspection of fixtures confirms they use production EditorLayer/panels, actual GL/ImGui and native dialogs, with always-active exception oracles; no Release-disabled assertion supplies the behavioral pass. Regression creates a private desktop before UI and fails closed if isolation cannot initialize. The desktop is never switched to the user's input desktop. These fixtures do not certify arbitrary interactive workflows, glyph coverage, or excluded defects.

| Observation | Recorded result in both builds | Evidence scope |
| --- | --- | --- |
| New scene / Save As / reopen | New destination saved and supported component values reopened | `editor-issue-15`, production file operations and native dialog automation |
| Unicode Save As / Open | Path and scene preserved; missing Open rejected | Same fixture; no general Unicode font/path API guarantee |
| Cancel / overwrite | Prompt appeared; decline/cancel preserved bytes; confirmation replaced scene | Same fixture; over-capacity paths also rejected without writing/crashing |
| Fractional viewport | Stable 800.5 × 600.5 panel retained 800 × 600 target and attachment IDs | `viewport-pixels` / `editor-issue-17` |
| Zero / negative / subpixel viewport | Usable framebuffer and camera retained; New/Open used pixels or last usable dimensions | Same fixture; changed pixels reached target, events and camera, style minimum restored |
| Long / percent tags | Empty, percent, UTF-8, 255/256 and 8192+ byte input retained; literal labels and row IDs checked | `entity-tags` / `editor-issue-16`, actual hierarchy ImGui input |
| Selection / deletion | Rename retained expansion/selection; blank deselect, reselect and Delete Entity menu passed | Same fixture |
| Minimize / restore | Debug: 3.00097 s minimized, 0 fixed updates, 180 ImGui frames; Release: 3.00041 s, 0, 180 | `minimize-restore` / `editor-issue-18`, real Win32 minimize/restore |
| Restore pacing | First restore frame 0 updates; next 0.250034 s Debug / 0.250006 s Release gave 30 updates at 1/120 s | Same fixture, exercises real Application frame policy |
| Scene API editing / rendering | Production panels rendered tagged Transform/Sprite/Camera/Visibility entity, one quad, matching camera/target; fixture edited tag, removed components and entity | `editor-issue-12`; component edits use the scene API and do not establish ordinary Inspector editing input |
| Ordinary Inspector editing input | Unrun for this repair; no supplied pass | New `entity-tags` / `editor-issue-16` checks add Camera and Visibility via the actual menu, toggle Primary and visibility via production widgets, verify registry changes and retain selection before hierarchy deletion |

All five standalone scripts passed in both builds at the recorded #25 SHA. No human manual observations are supplied; those remain unrun, as does the new Inspector-input coverage in this repair. The Windows host executes acceptance for the repair and can append matching evidence after review. Do not mark a manual gate passed from screenshots in the READMEs or a source search.

## Focused coverage and limitations

F01–F14 map to selectors in [the audit inventory](../codebase-improvement-plan.md). Renderer batch tests read actual rendered pixels, texture checks read GL storage, hierarchy tests use actual ImGui; native dialog automation is bounded. Scene hooks retain unsupported-type diagnostics limitations. Regressions do not authorize enabling dormant systems, simultaneous scenes, transactional deserialization, generalized Release failure handling, or teardown redesign. [The audit exclusions](../codebase-improvement-plan.md#excluded-findings-retained-from-the-audit) remain visible after these passes.

## Input fingerprints

SHA-256 identifies the supplied evidence files used by this publication; this is provenance, not a replacement for logs or a future run:

| File in the supplied #25 record | SHA-256 |
| --- | --- |
| `results.json` | `afab0d1e32cf796acc9232eb00b392c2e41bfb2e4e39d1fdea3d46987ee61ac5` |
| `results.md` | `7ba996a077436078565827c61cd8ca48549c7ae7982f1d3a22752957cd4b37fd` |
| `plan.json` | `8b0e933d75cbffe4732efcc2ec7677ebd502625bdccc859693be05da8fea2b07` |
| `generate.log` | `dd1ad43db5520749afb4fe1c482da6df9399222a0940240846cd97e94a283aae` |
| `build-Debug.log` | `de5b1e55b03416e73eda899348dd2df1266d18f75fa9c5ada4fa2e63df5d50cd` |
| `build-Release.log` | `01861b73241ba6c51d4e7514ce7466348ebbfa8dcf7c5fd57bfb454c0c09aa38` |
| `regression-all-Debug.log` | `7279de2f27669852322aff7845d392059c5a39dbdd2bf533e8644067d6d63a18` |
| `regression-all-Release.log` | `22fd6ed590c9ddaa835a0fbf2338659718d69e7a4ef83c3484c8eeffb7d2e106` |

## Local #25 checks

The initial publication reviewed all listed contract seams and their implementations, reconciled both local drafts, verified local Markdown targets and ran `git diff --check`. Its C++ changes were comment-only. This repair preserves those contracts and adds bounded, always-active Inspector-input assertions to the existing real ImGui panel fixture; it does not change production behavior, source inclusion or selector names. Local checks are recorded in [Tests/README.md](../../Tests/README.md#combined-validation-for-issue-25). Windows generation/build/runtime is unrun for this repair. No vendor implementation, generated output, public name, C++20/Premake/MSVC configuration, submodule pin or host scheduling file was changed; dependencies remain read-only, without downloads.
