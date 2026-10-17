# Combined validation evidence for specification #11 / issue #25

## Provenance and status

The supplied Windows record `issue-25-2f06a8e8bcd59e45e7a459eded58993b547bae5a-attempt-3` validates **2f06a8e8bcd59e45e7a459eded58993b547bae5a**, the #25 documentation reconciliation containing the Inspector-coverage repair on accepted base **e3bf3fff0f651b072b1357f234b853b3e2a3eea6**. The input files are available under `/opt/editor-validation/`; raw logs and generated artifacts are not repository deliverables. `results.json` reports passed: all 41 steps exit 0, no timeouts, markers present. This reconciliation cross-checked every focused and standalone-script log and all full-suite markers. It supersedes the first #25 publication record and earlier #24 record for combined-validation status; no pass is inferred from draft documentation.

The supplied run includes the Inspector input coverage added to `entity-tags` / `editor-issue-16`: both configurations passed menu and checkbox assertions against production ImGui widgets. This documentation-only reconciliation changes no production code, tests, source inclusion or build configuration. The recorded passes apply to **2f06a8e8bcd59e45e7a459eded58993b547bae5a**. Final acceptance of this reconciliation is recorded separately in the matching-commit host report described below. Human interactive ordinary editing remains unrun.

| Supplied step | Debug | Release | Scope |
| --- | --- | --- | --- |
| Premake generation | Exit 0 (one shared generation) | Same projects | Existing Windows VS2022 configuration |
| Solution Rebuild | Exit 0 | Exit 0 | Engine, Editor and Regression together at the recorded #25 SHA |
| Regression with no arguments | Exit 0, all markers | Exit 0, all markers | Complete suite including genuine editor fixtures and native dialogs |
| Each of the 13 configured selectors | Exit 0, matching PASS | Exit 0, matching PASS | All focused selectors listed below |
| Five separate issue scripts | Exit 0, matching PASS | Exit 0, matching PASS | `issue-12.ps1`, `issue-15.ps1`, `issue-16.ps1`, `issue-17.ps1`, `issue-18.ps1`, each launched separately |
| Human interactive Editor session | No supplied observation | No supplied observation | Automated fixture evidence below is narrower |
| Inspector-input repair (included at `2f06a8e`) | Passed in supplied run | Passed in supplied run | Included in `entity-tags`, full suite and standalone `issue-16.ps1` |

## Final acceptance record

The checkpoint above is a completed historical run, not the identity of every
later documentation commit. The Windows host writes the final reconciliation's
report to `.sandcastle/logs/spec-11/issue-25-final/results.json` and `results.md`
in the Editor checkout, then publishes that report and the full reviewed SHA
on [issue #25](https://github.com/AAAiee/GameFramework/issues/25). The report's
`commit` field must equal the candidate HEAD; `status` must be `passed`, and all
41 checks must exit 0 without timeouts and with required markers present. The
reviewer receives that exact report. A previous checkpoint cannot substitute
for it. Human interactive checks remain unrun.

The final run uses this command from `C:\FinalPolishedProjects\Editor`:

```powershell
pwsh.exe -NoProfile -File Scripts/Validate-Windows.ps1 `
    -Worktree C:/FinalPolishedProjects/Editor-validate-12 `
    -BuildTarget Rebuild `
    -PlanPath .sandcastle/logs/spec-11/issue-25-2f06a8e8bcd59e45e7a459eded58993b547bae5a-attempt-3/plan.json `
    -ReportPath .sandcastle/logs/spec-11/issue-25-final/results.json
```

It executes the same commands, selectors and scripts expanded below. Its exact
commit and outcomes are captured after execution in the host report and issue
comment, which can record a commit without changing that commit. Keeping final
acceptance there avoids a self-referential requirement to embed a document's
own Git hash in its contents. No pass is inferred before this report exists.

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
| Minimize / restore | Debug: 3.00103 s minimized, 0 fixed updates, 180 ImGui frames; Release: 3.00042 s, 0, 180 | `minimize-restore` / `editor-issue-18`, real Win32 minimize/restore |
| Restore pacing | First restore frame 0 updates; next 0.250026 s Debug / 0.250007 s Release gave 30 updates at 1/120 s | Same fixture, exercises real Application frame policy |
| Scene API editing / rendering | Production panels rendered tagged Transform/Sprite/Camera/Visibility entity, one quad, matching camera/target; fixture edited tag, removed components and entity | `editor-issue-12`; component edits use the scene API and do not establish ordinary Inspector editing input |
| Ordinary Inspector editing input | Camera and Visibility added; Primary toggled, visibility hidden/restored; selection retained | `entity-tags` / `editor-issue-16` drives the actual Add Component menu and production checkbox widgets, with always-active registry and selection assertions; not exhaustive Inspector coverage |

All five standalone scripts passed in both builds at the recorded #25 SHA. Inspector observations also appear in both focused `entity-tags` logs; both standalone `windows-issue-16` logs record exit 0 and the matching pass marker. No human manual observations are supplied; those remain unrun. The Windows host owns any subsequent acceptance execution and can append matching evidence after review. Do not mark a manual gate passed from screenshots in the READMEs or a source search.

## Focused coverage and limitations

F01–F14 map to selectors in [the audit inventory](../codebase-improvement-plan.md). Renderer batch tests read actual rendered pixels, texture checks read GL storage, hierarchy tests use actual ImGui; native dialog automation is bounded. Scene hooks retain unsupported-type diagnostics limitations. Regressions do not authorize enabling dormant systems, simultaneous scenes, transactional deserialization, generalized Release failure handling, or teardown redesign. [The audit exclusions](../codebase-improvement-plan.md#excluded-findings-retained-from-the-audit) remain visible after these passes.

## Input fingerprints

SHA-256 identifies the supplied evidence files used by this publication; this is provenance, not a replacement for logs or a future run:

| File in the supplied #25 record | SHA-256 |
| --- | --- |
| `results.json` | `68cea177b4485d6a5ac54c1539ec2abf136f0471227db9bf9b7c87f7169bfd9c` |
| `results.md` | `20c02996d96f29b46ca6b608091e5fd39054d2d31f63470115e1d8e54c51689e` |
| `plan.json` | `8b0e933d75cbffe4732efcc2ec7677ebd502625bdccc859693be05da8fea2b07` |
| `generate.log` | `cad4b70ef114c2f4be0dea8fce59d47818130bb409fee27261d4e93475cf0574` |
| `build-Debug.log` | `0a4c89ac0977e3ab5c2a81fc6dc4d8c3c2354816db6cdbc7702509bf93748ebc` |
| `build-Release.log` | `a76975e44433b7428c8893f57c05473439564417072e6bae9ef1074872396584` |
| `regression-all-Debug.log` | `b0d4338b2ffc7b224455096a123addd0a38b1b8a0f8c8da0c05e10663c6e9317` |
| `regression-all-Release.log` | `2187d412d7f047712f36c41917c7d89dec0dcd5ade9a68af87e8b2cc91342ea5` |

## Local #25 checks

The initial publication reviewed all listed contract seams and their implementations, reconciled both local drafts, verified local Markdown targets and ran `git diff --check`. Its C++ changes were comment-only. The Inspector-coverage repair preserved those contracts and added bounded, always-active assertions to the existing real ImGui panel fixture; its Windows generation/build/runtime passed in the supplied run above. This subsequent reconciliation changes only documentation and fingerprints. Local checks reverified all 41 supplied steps, every full-suite/focused/script marker, Inspector observation lines and evidence fingerprints, checked local Markdown file targets, and ran `git diff --check`. No Windows command was executed in this Linux container. Historical local compile checks are recorded in [Tests/README.md](../../Tests/README.md#combined-validation-for-issue-25). No vendor implementation, generated output, public name, C++20/Premake/MSVC configuration, submodule pin or host scheduling file was changed; dependencies remain read-only, without downloads.
