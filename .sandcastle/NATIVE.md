# Native diagnostics and acceptance

`createEditorNativeValidation` in [native.mjs](native.mjs) supplies explicit
`prepareValidation`, `diagnose`, `validate` and evidence callbacks to the task
adapter. The consumer supplies absolute PowerShell 7 and Visual Studio 2022
MSBuild paths and initialized local dependency sources. This does not start the
paused runner, select a task, invoke a model or enqueue work.

Diagnostics run before final validation. They generate projects, use incremental
Debug `Build`, and run the task's known selectors and dependent fixtures. For
example, font-atlas diagnostics include atlas-placeholders, and GL-buffer
diagnostics include GL-boundaries. Unknown coverage uses the complete Debug
plan: full Regression, all thirteen standalone selectors, five private-desktop
UI scripts and the tracked UnitTests procedure. Failed checks return their exact
check names, concise diagnostics and log/report locations to bounded repair.

Final validation always uses Debug and Release `Rebuild`, both full Regression
suites, all standalone selectors and all five UI scripts in both configurations.
When `Tests/Build-UnitTests.lua` is tracked, both configurations also require
normal and filtered runs, deliberately failing console/JUnit probes, and a
rebuilt passing runner. A supplied plan cannot remove these gates. Passing
diagnostics cannot substitute for final acceptance. Host reports retain actual
per-check durations separately from outer workflow measurements.

Preparation uses an owned runtime resource and independently cloned local
submodules at the candidate's exact pins. Git file transport and no-lazy-fetch
settings prevent implicit network downloads. Existing prepared build outputs
are reused; source and prepared staged, unstaged and untracked edits are
preserved through the public prepared-worktree boundary.

Native commands use the installed package's owned Windows JobObject runner.
The driver waits for its checks before exiting; `completion: "primary"` then
stops remaining owned tool-server descendants and confirms zero active job
processes. MSBuild node reuse is disabled and each driver uses a private MSVC
PDB endpoint so an existing external tool server cannot inherit its writes.
Cancellation and timeout require confirmed whole-job shutdown before checkout
restoration. Unknown cleanup preserves the candidate, recovery record and runtime
lock for explicit reconciliation. No existing unrelated worker is terminated.
The portable process entry currently supports Windows 10/Server 2016 or newer
with an explicit PowerShell 7 executable; unsupported hosts fail before launch.

Private-desktop isolation and success-marker assertions remain fail closed.
The disposable UnitTests probe uses a checked short directory under the owned
prepared `Binaries` directory; it never changes the tracked runner. Validation
inputs include current plans, driver/inspector/adapter/process-boundary bytes,
installed package and prepared dependency observations, and actual host tools.
Inputs must match before and after a run for its evidence to remain eligible.

Native reports apply only to their recorded candidate and observed inputs.
Deterministic workflow tests and a focused native procedure do not establish
final Debug/Release acceptance or live agent reliability. Final integration
verification must use the deliberately pinned combined package.
