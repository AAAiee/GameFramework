# Continuous Windows acceptance

Run from the Editor checkout in Windows PowerShell with Node 22.13+:

```powershell
npm run sandcastle:check -- --offline
npm run sandcastle
```

The npm entry uses the installed Node runtime directly; it does not use npx or
download tsx. A prepared `sandcastle:editor` Docker image, dedicated Codex login,
PowerShell 7, GitHub CLI, MSVC 2022, and an interactive Windows desktop with a
compatible OpenGL driver are required. The online check also reads GitHub without
starting agents or changing issues. GitHub/model access still needs connectivity.
Host GitHub requests honor explicit proxy environment variables, otherwise the
enabled Windows system HTTP/HTTPS proxy. Container `gh` uses the same proxy,
translating host loopback to `host.docker.internal`. Its wrapper scopes proxy
variables to GitHub CLI, preserving the existing model-service route.

`workflow-config.mjs` defines spec #11's scope, dependencies, selectors and UI
checks. The host picks an eligible issue, runs the implementer, validates its
committed candidate in Windows, then gives matching-SHA evidence to a read-only
reviewer. After both gates pass it fast-forwards Editor, records evidence,
closes that issue, and continues. The configured tasks include #12 even though it
was previously handed off without its Sandcastle label. Its existing reviewed
branch is used as a seed. #25 is withheld until #13–#24 are closed, then enqueued
automatically. The parent issue remains unchanged.

One agent container/branch is reused throughout a run. The existing
`../Editor-validate-12` checkout is retained for Windows builds and moved to each
reviewed commit in detached-HEAD mode, keeping its local dependencies and build
cache. Nothing clones, initializes submodules, pulls images, or installs packages.
Git network transports/lazy fetching are disabled. A missing dependency or changed
pin requires local preparation rather than silently downloading it.

The issue-specific selectors and Windows scripts are acceptance contracts. Agents
must supply real checks in the implementation; missing scripts/selectors fail the
gate. The host rebuilds Debug/Release, runs the entire Regression suite and all
configured focused checks, then any required Windows UI scripts in both builds.
#25 reruns the combined selectors and UI scripts. Passing compilation alone never
closes an issue with pending UI acceptance. Check contracts are in the prompts;
Windows execution and logs are owned by Scripts/Validate-Windows.ps1.

Each task gets up to three implementation/validation/review attempts. Failed logs
are returned to the implementer. Limits and run duration are in the config. All
agents must commit; a review result must identify the final SHA and account for
every issue checklist criterion. A changed issue body invalidates resumed work.

Progress lives in ignored `.sandcastle/state/workflow.json`; reports and agent
logs live in `.sandcastle/logs/spec-11`. Network errors stop with state preserved.
Rerun the normal command to resume; after exhausting the repair budget, inspect
the failure and use `npm run sandcastle -- --retry` to grant another bounded set.
Merged tasks resume their evidence publication/closure without implementing again.
After a Windows pass, a saved review checkpoint also resumes without rebuilding;
the host verifies the saved report and branch still identify the same commit.
GitHub requests allow 120 seconds each and retry transient network/server failures
up to three attempts with backoff. Comment POSTs are reconciled against their
commit-specific marker before resending: a lost response can mean the comment
already exists. Unavailable reconciliation reads stop publication safely. These
network retries do not consume implementation attempts or rerun accepted builds.

The main Editor checkout can have local edits. Before a fast-forward merge, the
runner saves tracked edits in a Git stash and restores them with `apply --index`;
the recovery stash is retained and recorded. Untracked files are never stashed
or deleted, and a collision stops the merge. If restoration conflicts or a crash
leaves `pendingStash`, inspect the saved SHA and resolve restoration before
clearing that checkpoint. Neither conflicts nor target-branch divergence are
automatically reset. No remote push is performed; comments identify local merges.

A lock prevents overlapping runs. A hard process kill may leave the lock: check
the PID in `state/owner.json` before removing only that stale lock. A preserved
dirty agent worktree also requires inspection before restarting; it is never
silently discarded. Run `npm run sandcastle:test` for the local orchestration and
Git-preservation tests. These tests do not invoke agents or change GitHub issues.
