# Combined validation evidence for specification #11 / issue #25

## Provenance and status

The supplied Windows record `issue-25-5cfa6a3613281049e4f20dbaab238bddbb678f99-attempt-2` validates **5cfa6a3613281049e4f20dbaab238bddbb678f99**, the #25 Inspector-coverage repair commit on accepted base **e3bf3fff0f651b072b1357f234b853b3e2a3eea6**. The input files are available under `/opt/editor-validation/`; raw logs and generated artifacts are not repository deliverables. `results.json` reports passed: all 41 steps exit 0, no timeouts, markers present. This repair cross-checked every focused and standalone-script log and all full-suite markers. It supersedes the first #25 publication record and earlier #24 record for combined-validation status; no pass is inferred from draft documentation.

The supplied run includes the Inspector input coverage added to `entity-tags` / `editor-issue-16`: both configurations passed menu and checkbox assertions against production ImGui widgets. This documentation-only reconciliation changes no production code, tests, source inclusion or build configuration. The recorded passes apply to **5cfa6a3613281049e4f20dbaab238bddbb678f99**, not a claimed execution of this later documentation commit. Human interactive ordinary editing remains unrun.

| Supplied step | Debug | Release | Scope |
| --- | --- | --- | --- |
| Premake generation | Exit 0 (one shared generation) | Same projects | Existing Windows VS2022 configuration |
| Solution Rebuild | Exit 0 | Exit 0 | Engine, Editor and Regression together at the recorded #25 SHA |
| Regression with no arguments | Exit 0, all markers | Exit 0, all markers | Complete suite including genuine editor fixtures and native dialogs |
| Each of the 13 configured selectors | Exit 0, matching PASS | Exit 0, matching PASS | All focused selectors listed below |
| Five separate issue scripts | Exit 0, matching PASS | Exit 0, matching PASS | `issue-12.ps1`, `issue-15.ps1`, `issue-16.ps1`, `issue-17.ps1`, `issue-18.ps1`, each launched separately |
| Human interactive Editor session | No supplied observation | No supplied observation | Automated fixture evidence below is narrower |
| Inspector-input repair (`5cfa6a3`) | Passed in supplied run | Passed in supplied run | Included in `entity-tags`, full suite and standalone `issue-16.ps1` |

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
| Minimize / restore | Debug: 3.00093 s minimized, 0 fixed updates, 180 ImGui frames; Release: 3.00045 s, 0, 180 | `minimize-restore` / `editor-issue-18`, real Win32 minimize/restore |
| Restore pacing | First restore frame 0 updates; next 0.250059 s Debug / 0.250006 s Release gave 30 updates at 1/120 s | Same fixture, exercises real Application frame policy |
| Scene API editing / rendering | Production panels rendered tagged Transform/Sprite/Camera/Visibility entity, one quad, matching camera/target; fixture edited tag, removed components and entity | `editor-issue-12`; component edits use the scene API and do not establish ordinary Inspector editing input |
| Ordinary Inspector editing input | Camera and Visibility added; Primary toggled, visibility hidden/restored; selection retained | `entity-tags` / `editor-issue-16` drives the actual Add Component menu and production checkbox widgets, with always-active registry and selection assertions; not exhaustive Inspector coverage |

All five standalone scripts passed in both builds at the recorded #25 SHA. Inspector observations also appear in both focused `entity-tags` logs; both standalone `windows-issue-16` logs record exit 0 and the matching pass marker. No human manual observations are supplied; those remain unrun. The Windows host owns any subsequent acceptance execution and can append matching evidence after review. Do not mark a manual gate passed from screenshots in the READMEs or a source search.

## Focused coverage and limitations

F01–F14 map to selectors in [the audit inventory](../codebase-improvement-plan.md). Renderer batch tests read actual rendered pixels, texture checks read GL storage, hierarchy tests use actual ImGui; native dialog automation is bounded. Scene hooks retain unsupported-type diagnostics limitations. Regressions do not authorize enabling dormant systems, simultaneous scenes, transactional deserialization, generalized Release failure handling, or teardown redesign. [The audit exclusions](../codebase-improvement-plan.md#excluded-findings-retained-from-the-audit) remain visible after these passes.

## Input fingerprints

SHA-256 identifies the supplied evidence files used by this publication; this is provenance, not a replacement for logs or a future run:

| File in the supplied #25 record | SHA-256 |
| --- | --- |
| `results.json` | `5f61faaf0bd190085dcfa8d4b153a576dc383dd1b44eb84c6ab4cc37fccf5d37` |
| `results.md` | `e9d47c447402ce9a73ec64f5b63998cb72049fc1a33d34835022cfe0a050bbba` |
| `plan.json` | `8b0e933d75cbffe4732efcc2ec7677ebd502625bdccc859693be05da8fea2b07` |
| `generate.log` | `1dfac1dac643f346a3538d02ef1a804c516a6364ed36f09187d3a5b843a59168` |
| `build-Debug.log` | `15811af613453ea328c1b81f196326de210accb40682aa701718220737c4fdbb` |
| `build-Release.log` | `b3d75e3852109994d67666015ace33a6f1cf3a15ab520bb5e27a11f03c53f926` |
| `regression-all-Debug.log` | `d8a1e7d8ef2b43815d4bc3771774dbe58f5971f3b881a265801f8f06cea76edd` |
| `regression-all-Release.log` | `ec51070829409b35b86e9b4a9319476185fa0f7bd87da4dc2849b70bb28d07cb` |

## Local #25 checks

The initial publication reviewed all listed contract seams and their implementations, reconciled both local drafts, verified local Markdown targets and ran `git diff --check`. Its C++ changes were comment-only. The Inspector-coverage repair preserved those contracts and added bounded, always-active assertions to the existing real ImGui panel fixture; its Windows generation/build/runtime passed in the supplied run above. This subsequent reconciliation changes only documentation and fingerprints. Local checks reverified all 41 supplied steps, every full-suite/focused/script marker, Inspector observation lines and evidence fingerprints, checked local Markdown file targets, and ran `git diff --check`. No Windows command was executed in this Linux container. Historical local compile checks are recorded in [Tests/README.md](../../Tests/README.md#combined-validation-for-issue-25). No vendor implementation, generated output, public name, C++20/Premake/MSVC configuration, submodule pin or host scheduling file was changed; dependencies remain read-only, without downloads.
