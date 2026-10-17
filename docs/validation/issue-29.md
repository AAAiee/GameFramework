# Issue #29 validation

Implementation base: `ea6339e457ab1758d427b0e6582b0d6f68521808`.
These are Linux implementation checks on 2026-10-17, not Windows acceptance.
The Windows host owns scheduling, native testing, PR delivery and merges.

## Executed locally

The unmodified upstream doctest header reports version 2.4.12. SHA-256 checks
matched both files recorded in [the vendor manifest](../../Vendor/doctest/README.md).
Git attributes preserve their upstream bytes on Windows checkouts.

GCC 12.2.0 compiled the checked-in runner and shared CameraInverseCheck.cpp,
using only the vendored doctest, Tests/src, Engine/Source and read-only
`/opt/editor-dependencies/glm` include directories. No Engine source/library,
graphics/windowing/editor library or portability shim was required:

```text
g++ -std=c++20 <configuration flags> -IVendor/doctest -ITests/src \
    -IEngine/Source -I/opt/editor-dependencies/glm \
    Tests/unit/UnitTests.cpp Tests/src/CameraInverseCheck.cpp -o <temporary binary>
```

| Configuration flags | Passing console run | Temporary `CHECK(false)` run | Filtered JUnit run |
| --- | --- | --- | --- |
| `-O0 -g` | exit 0; 1 case, 1 assertion passed | exit 1; 1 passed, 1 failed assertion | exit 0; tests=1, failures=0, errors=0 |
| `-O2 -DNDEBUG` | exit 0; 1 case, 1 assertion passed | exit 1; 1 passed, 1 failed assertion | exit 0; tests=1, failures=0, errors=0 |

Passing console runs used `--no-colors --no-breaks`. JUnit runs used
`--test-case=camera-inverse --reporters=junit --out=<temporary report>`;
both XML reports were parsed and their case/failure counts checked.

The deliberately failing assertion was added only to a temporary runner copy,
immediately after the shared check. Both logs reported
`ERROR: CHECK( false ) is NOT correct!` and `Status: FAILURE!`, exit 1.
The repository runner was never modified for the probe. The temporary probe
source/binaries were removed after verification; passing binaries were rerun.
Local logs/reports were retained in `/tmp/issue-29-checks-_m8kle59` for this session.

VS2022 project generation passed using the repository's Linux Premake binary in
a temporary repository copy with prepared dependency snapshots. XML inspection
confirmed exactly the runner and shared check as UnitTests compile sources,
`stdcpp20`, `MultiThreadedDebug` / `MultiThreaded` for Debug / Release, no link
dependencies and no project references. Regression's generated project excludes
the runner. This does not run MSVC. `git diff --check` also passed.

## Required host acceptance (pending)

Follow [Tests/README.md](../../Tests/README.md#headless-unittests-issue-29).
Record the candidate commit and x64 Debug/Release generation/build logs, passing
console/filter/JUnit runs, deliberately failing assertion nonzero exits in both
configurations, then removal/rebuild/passing runs. Linux optimized execution does
not establish Windows CRT linkage or MSVC behavior.

Run the existing full Regression suite, all 13 configured selectors and the
five configured Windows scripts in both configurations. No Windows build,
Regression, private-desktop UI run or manual Editor check was executed here.
Keep issue-tracker writes, reviewed PR delivery and merges with the host; issues
remain open until merge. No workflow, package or branch-protection changes are
part of this implementation.

## Bounded repair after first review

The supplied Windows report for candidate
`12e229a031ce2d69ee0e6fa5bf7493dfc47cdd1a` is available at
`/opt/editor-validation/issue-29-12e229a031ce2d69ee0e6fa5bf7493dfc47cdd1a-attempt-1/results.json`.
Read-only inspection confirmed 41 stages with exit 0, no timeouts and their
required markers: generation, both solution builds, both complete Regression
runs, all 13 selectors and all five UI scripts in both configurations. This is
prior-candidate evidence, not acceptance of the repair commit. The plan contains
no UnitTests executions; it cannot establish doctest Release failure behavior.

The repair adds `Tests/Windows/unit-tests.ps1` and the exact invocation in
Tests/README.md. The host must add these two explicit Debug/Release invocations
to its candidate validation; the existing five UI scripts do not invoke them.
The script records the candidate SHA and stage exit codes, verifies passing
console and filtered JUnit runs, compiles a disposable generated-project copy
with a failing assertion, requires assertion-specific output plus exit 1 and
failed JUnit evidence, removes the probe, then rebuilds/reruns the normal target.
No tracked source is temporarily modified. Probe project placement preserves
the generated project's relative paths; output overrides isolate failing code
and binaries from the ordinary executable. Cleanup runs even on failure.

Local repair validation uses GCC C++20 with `-O0 -g` and `-O2 -DNDEBUG`, the
unchanged checked-in runner/shared check and a temporary runner copy containing
`CHECK(false)`. Both normal console/filter runs exit 0; both failing console
and JUnit runs exit 1 and identify the intended assertion. Parsed XML contains
one `camera-inverse` testcase in each run, zero errors, and the expected zero or
one failures. In doctest 2.4.12 the JUnit `tests` attribute counts assertions:
one normally and two in the probe; the script checks the testcase separately.
The passing binaries were rerun, and temporary probe sources/binaries removed.
Logs/XML: `/tmp/issue-29-repair-2dk9syza`.

This Linux environment has neither PowerShell nor MSVC. The new PowerShell
script and native acceptance remain unexecuted here. C5 Windows evidence is
still pending: retain both `UnitTests/issue-29` log/report directories and the
matching final candidate SHA when the host executes the documented commands.
Existing Regression/UI scripts, isolation, dependencies, vendor files, workflows
and the owner's #28 work are unchanged by this repair. `git diff --check` passed.

## Second review: host execution blocker

The second supplied report identifies candidate
`38cec3eba9b66d780f8074118a56584f6cf6d1ab` in
`/opt/editor-validation/issue-29-38cec3eba9b66d780f8074118a56584f6cf6d1ab-attempt-2/results.json`.
Read-only inspection confirms all 41 checks exited 0, did not time out and
satisfied their markers. Its `plan.json` still lists only the 13 Regression
selectors and five UI scripts. Neither the plan nor the results includes
`Tests/Windows/unit-tests.ps1`, a UnitTests run, or a failure-probe run.
The report's overall PASS therefore does not satisfy C5. The first repair's
script was not executed by this host plan; no native script failure is reported.

The remaining correction belongs to host scheduling. After generation, the host
must execute the following **additional** stages in its prepared candidate
worktree, sequentially, using its existing PowerShell and MSBuild executables:

| Stage | Script arguments after `-NoProfile -File Tests/Windows/unit-tests.ps1` | Required output and exit |
| --- | --- | --- |
| `unit-tests-Debug` | `-Configuration Debug -BinaryRoot <absolute worktree>/Binaries/windows-x86_64/Debug -MSBuild <absolute MSBuild.exe>` | `PASS: unit-tests-Debug`, exit 0 |
| `unit-tests-Release` | `-Configuration Release -BinaryRoot <absolute worktree>/Binaries/windows-x86_64/Release -MSBuild <absolute MSBuild.exe>` | `PASS: unit-tests-Release`, exit 0 |

Quote absolute paths containing spaces. Allow 900 seconds for each outer stage:
the script independently bounds its three builds to 180 seconds each and six
executable runs to 30 seconds each. A normal 41-stage host report plus these two
stages has 43 stages. Keep the existing Regression/UI stages and their isolation
unchanged. Preserve each configuration's `UnitTests/issue-29` evidence directory,
including the candidate SHA, passing runs, failing console/JUnit runs and final
passing runs. Require both script markers and zero exits, and review the nested
logs against C5 before claiming acceptance. Run these stages against the final
candidate commit, not only the earlier implementation SHA.

The Linux implementer cannot run MSVC or Windows PowerShell here and cannot
establish C5 by another local run. Host scheduling, AFK workflow files and
issue-tracker state remain outside this repair. No assertion is weakened and no
Windows pass is claimed. This handoff documents an external execution blocker;
it does not resolve C5. The implementation and acceptance script are preserved.

Local handoff validation: parsed the supplied plan/results and checked its SHA,
all 41 exits, timeout flags and markers, and the absence of UnitTests stages;
verified both vendored SHA-256 checksums, affected Markdown file targets and
`git diff --check`. This repair changes only documentation.
