# Assigned task

{{TASK_CONTEXT}}

Work on this issue only, on branch `{{BRANCH}}`. The accepted base is `{{BASE}}`.
The Windows host owns scheduling, testing, merging, labels, comments, and closure.
Leave all issue-tracker writes and branch merges to that host.

# Previous attempt

{{FEEDBACK}}

# Process

1. Read the issue's criteria and the relevant production code. Preserve the
   current branch's earlier work; #12 may already contain the reviewed repair.
   Read the linked parent specification if the assigned body leaves scope unclear.
2. Implement the bounded correction and its meaningful production-code checks.
   Use the existing C++20/Premake/MSVC configuration and extend Regression.
   Keep every configured selector available. `Regression.exe` with no arguments
   runs the complete suite; a selector runs its named check, prints `PASS: <name>`
   only after it passes, and exits 0. Failed checks exit nonzero in both builds.
3. For each configured Windows check, implement its exact `Tests/Windows/...ps1`
   path. Accept `-Configuration Debug|Release` and `-BinaryRoot <absolute path>`.
   Exercise the real Editor/window/dialog behavior required by the issue using
   Windows UI automation or a genuine editor test fixture. Use the output folder
   for working files, handle timeouts, and close only processes the test started.
   Print its configured marker only after behavioral assertions pass; otherwise
   throw or exit nonzero. Source searches, sleeps alone, mocked production paths,
   and unconditional success markers are not runtime evidence.
   Preserve Regression's private-desktop isolation: modal tests must complete
   without user input or windows on the user's input desktop. Fail closed when
   isolation or automation cannot initialize, and keep process timeouts bounded.
4. Update affected API contracts and Tests/README.md. Read-only dependencies are
   under `/opt/editor-dependencies`; local documentation drafts are under
   `/opt/editor-local-docs`. Existing Windows failure logs are under
   `/opt/editor-validation`. Reuse these inputs; dependency downloads are disabled.
5. Inspect the patch and commit it with `RALPH: #<number> <change>` (repair commits
   may be additional commits). Leave a clean working tree. Preserve the parent
   specification's exclusions and all previously accepted checks.

This Linux container cannot run authoritative Windows acceptance. The host will
execute the configured build, regression, and UI scripts, then return failures
here for a bounded repair attempt. Report genuine blockers rather than weakening
an assertion or claiming an unexecuted check passed.

Keep `.sandcastle/`, the host validation script, package files, submodule pins,
and vendor implementations unchanged. The reviewer checks code and test quality;
the host requires the configured success markers and exit codes independently.

Finish after committing the assigned implementation and its acceptance checks:
<promise>COMPLETE</promise>
