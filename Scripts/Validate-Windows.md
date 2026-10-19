# Windows native driver reference

Operational setup, preparation, supervised reuse and report interpretation:
[Tests/recipes/native-validation.md](../Tests/recipes/native-validation.md).
The driver runs against an already prepared **clean** worktree; it does not
prepare dependencies or provide the workflow adapter's owned-process boundary.

## Parameters

| Parameter | Contract |
| --- | --- |
| `-Worktree` | Required prepared worktree path |
| `-MSBuild` | Installed VS2022 MSBuild.exe path; when omitted, discover with vswhere |
| `-Phase` | `diagnose` or `validate`; default `validate` |
| `-BuildTarget` | `Build` or `Rebuild`; default `Rebuild`. Diagnose requires Build, validate requires Rebuild |
| `-PlanPath` | JSON from `.sandcastle/native.mjs`'s existing `selectNativePlan({phase,issue})` |
| `-ReportPath` | Output JSON path; default `<prepared>/Binaries/Validation/<timestamp>/results.json` |
| `-Selector` | Retained compatibility parameter; complete defaults or the supplied plan own selectors. It does not narrow the plan |

From the repository root, with absolute configured host paths:

```powershell
pwsh.exe -NoProfile -File .\Scripts\Validate-Windows.ps1 -Worktree $prepared `
  -MSBuild $msbuild -Phase diagnose -BuildTarget Build `
  -ReportPath $diagnosticReport
if ($LASTEXITCODE -ne 0) { throw 'Diagnostics failed; inspect the host report' }

pwsh.exe -NoProfile -File .\Scripts\Validate-Windows.ps1 -Worktree $prepared `
  -MSBuild $msbuild -Phase validate -BuildTarget Rebuild `
  -ReportPath $finalReport
if ($LASTEXITCODE -ne 0) { throw 'Final acceptance failed; inspect the host report' }
```

These are foreground driver API calls. Prefer `runEditorTask` with
`createEditorNativeValidation` for explicit JobObject supervision,
`completion: "primary"`, owned-tree quiescence and protected restoration.
Direct PowerShell cancellation alone does not establish that guarantee.

## Phase and report contracts

The driver generates projects on each invocation. Diagnostics default to the
complete incremental Debug Build plan; trusted coverage plans can focus checks.
Final validation enforces Debug/Release Rebuilds, each full Regression, all 13
standalone selectors, five UI wrappers and the tracked UnitTests
normal/filtered/deliberate-failure-probe script. Final plans may add coverage but
cannot subtract the mandatory baseline; diagnostic success is a separate gate.

Commands have bounded timeouts, markers and logs. The current outcome belongs to
`results.json`, sibling `results.md` and referenced raw logs. Reports identify
candidate, phase, configurations, build target, exits/timeouts/markers and actual
per-check `durationMs`; the adapter retains `nativeTimings` separately from
outer-stage measurements. Missing prerequisites, substantive check failures and
unknown process/report outcomes remain distinct.

[Inspect-NativeEnvironment.ps1](Inspect-NativeEnvironment.ps1) observes native
compiler, MSBuild, installed SDK headers, OS, graphics driver and Premake.
Docker package-build provenance is a separate identity. The driver disables
MSBuild node reuse and uses a private PDB-server endpoint; focused UnitTests does
the same. See the native recipe for private-desktop and recovery requirements,
and the fork's `docs/workflow-process.md` for generic ownership policy.
