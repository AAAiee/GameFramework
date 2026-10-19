# Editor test procedures

Start with [native validation](recipes/native-validation.md) for diagnostic and
final acceptance runs. Current outcomes belong to candidate-bound host reports
and their raw logs. This index describes stable procedures, not a current pass.

| Task | Recipe |
| --- | --- |
| Prepare resources, diagnose, run final acceptance, inspect reports | [Native validation](recipes/native-validation.md) |
| Run the full Regression suite or a behavior selector | [Regression](recipes/regression.md) |
| Exercise real Editor fixtures on a private desktop | [Editor UI](recipes/editor-ui.md) |
| Run the existing headless doctest target and failure probe | [UnitTests](recipes/unit-tests.md) |
| Check portable workflow behavior, provenance and frozen baseline | [Sandcastle](recipes/sandcastle.md) |
| Read original attempt narratives and evidence pointers | [Historical archive](HISTORY.md) |

## Prerequisites

Native checks require Windows, PowerShell 7, Visual Studio 2022 C++ x64 tools,
an installed Windows SDK, bundled Premake, initialized local dependency sources
at the candidate's pins, and a real OpenGL 4.5 compatible desktop/driver.
Discover tools on the host; do not assume a particular PowerShell installation
path. Portable workflow checks require Node 22+, npm and Git.

Use an explicitly prepared clean worktree and keep reports in ignored host run
logs outside tracked candidate source. The workflow's preparation API owns its
detached worktree and local dependency clones; preserve existing user checkouts.
See the native recipe before preparing or reusing resources.

## Commands

From the repository root, with `$prepared`, `$msbuild`, `$diagnosticReport` and
`$finalReport` set to absolute host paths, these are the foreground driver API
commands. For supervised process ownership and safe resource restoration, use
`runEditorTask` with `createEditorNativeValidation` as described in the native
recipe; direct PowerShell invocation alone does not provide that guarantee.

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

Diagnostics default to the complete Debug plan; mapped plans use the existing
`selectNativePlan` API and `-PlanPath` as shown in the recipe. Final acceptance
always requires Debug/Release Rebuilds, full Regression, all 13 selectors, five
UI wrappers and the tracked UnitTests acceptance script. A diagnostic pass does
not satisfy final acceptance.

Portable checks, from the repository root after installing the pinned packages:

```powershell
npm.cmd run sandcastle:check
npm.cmd run sandcastle:test
npm.cmd run test:sandcastle-baseline
```

See the [Sandcastle recipe](recipes/sandcastle.md) for boundaries and read-only
queue discovery. These commands establish neither native nor live model results.

## Evidence and historical records

`results.json`, `results.md`, per-command logs, process receipts and nested
UnitTests logs/JUnit own each host outcome. Preserve their original locations.
Candidate, policy, dependency and environment identity determine whether evidence
can be reused; see [.sandcastle/EVIDENCE.md](../.sandcastle/EVIDENCE.md).

[HISTORY.md](HISTORY.md) preserves the original README in full, including dated
passes, failures, unrun statements, raw log paths and URLs. Historical #25's
41-stage report and #29's pending host handoff describe named checkpoints; the
current plan includes the two UnitTests invocations (43 checks). Neither those
records nor this index authorize a later candidate.

## Headless UnitTests (issue #29)

Current procedure: [UnitTests](recipes/unit-tests.md). Original record:
[Headless UnitTests](HISTORY.md#headless-unittests-issue-29), including the
[historical second-review host blocker](../docs/validation/issue-29.md#second-review-host-execution-blocker).
This heading remains for existing incoming links.

## Combined validation for issue #25

Current procedure: [native validation](recipes/native-validation.md). Original
checkpoint: [combined validation](HISTORY.md#combined-validation-for-issue-25)
and [the spec-11 evidence ledger](../docs/validation/spec-11.md).
This heading remains for existing incoming links.
