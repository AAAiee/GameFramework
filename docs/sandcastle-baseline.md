# Sandcastle baseline preservation — issue #32

## Historical handoff and current rebuild

The sections below retain the original #32 handoff record. Its Linux container
paths, runtime exports and PR instructions describe that historical change;
they do not activate the historical runtime or authorize publication. The current
rebuild keeps the original fixture inactive and retains only its checksum verifier,
provenance and additive test command. The rebuilt package pins, read-only queue
and workflow adapter remain independently configured under `.sandcastle/`.

Fresh Windows verification on 2026-10-18, Node v22.13.0 and Git 2.47.1.windows.1,
verified all 15 original file checksums and passed all 32 original orchestration
tests. Run `npm run test:sandcastle-baseline` to verify the current checkout again.
Those deterministic tests use disposable Git repositories and simulated GitHub
responses; they do not establish native acceptance or live unattended reliability.
No native GUI, agent, old runner or remote publication is part of this command.
PR creation remains a human action.

## Original #32 preservation record

The original customization is retained as an inactive, runnable test/recovery
fixture at `Tests/fixtures/sandcastle-baseline`. It was restored from
`.scratch/issue-28-baseline` in supplied Editor input
`edb928f0dc2c17f200214b819bb632f14acaba0d`. Its 15 source files and original
manifest are preserved, including the original 22 workflow and 10 GitHub tests.
Git normalized six supplied blobs to LF (`.gitignore`, standards, implement/review
prompts, main entrypoint and package.json); their original CRLF bytes were
restored, matching every original manifest checksum. `.gitattributes` prevents
future checkout conversion of the recovery fixture. No behavioral reconstruction
was needed. This corrects the previous preparation blocker: the earlier input
`b48bc899c0eb2029e27f80e58ab2a4e9183e6ff5` did not contain this recovery snapshot.

The source manifest's `32 passed before extraction` is historical provenance.
Fresh verification on 2026-10-17, Linux, Node v22.23.3, ran all 32 original tests
successfully against the restored bytes. `npm run test:sandcastle-baseline`
rechecks the manifest and every file checksum before rerunning those tests.
The host owns current reports and Windows acceptance; no Windows/native check,
Docker/model run, live GitHub operation or live queue execution is claimed here.

## Reconciliation and ownership

| Input material | Disposition for this ticket |
| --- | --- |
| Original `workflow.mjs` hashing, criteria, structured review and state machine | Reusable implementations versioned in sandcastleCustomized `src/workflow/runner.mjs`; original bytes also retained in the recovery fixture. No recovery/evidence optimization imported. |
| Original `host.mjs` command/proxy helpers, validation worktree checks, safe merge and GitHub retries/reconciliation | Reusable implementations versioned in the fork, with publication text and close-label removal supplied explicitly. Temporary proxy path becomes generic. |
| Original issue selection/label exceptions and `validationPlan` | Remain in the Editor fixture with their three original tests; require explicit project adapters in subsequent extraction. |
| Original `main.mts`, `workflow-config.mjs`, prompts, standards, WORKFLOW, npm manifests and `Scripts/Validate-Windows.*` | Retained only in the inactive Editor recovery fixture. No change to active runner, native acceptance or package dependency. Historical WORKFLOW describes obsolete automatic delivery; do not execute it. |
| Partial Editor `main.mts`, host/workflow wrappers, workflow config and prompt changes | Retained in supplied input; not activated. They assume the unfinished extracted package and are not evidence of a restored baseline. |
| Partial Editor acceptance/dependencies/resources/runtime/task modules, Linux/package/Docker smoke tooling, queue tests, update-package script and review-format prompt | Deferred to their owning extraction/integration/optimization tickets. All remain available in the supplied input. |
| Partial Editor vendor tarball and package/lock changes | Deferred; the active dependency and vendored implementations are unchanged. |
| Partial fork README/package/changeset/asset-copy drafts and `src/workflow` runner/host/executor/supervisor/workspace drafts | Retained in fork input `fcec9e6d62837685d8dc6e56cef9b35712db8ccb`. Broader scheduler, recovery, evidence, workspace and supervisor behavior is not imported. Baseline runtime and asset-copy changes are separately reviewable replacements. |
| Editor `imgui.ini`, root standards/glossary and research/spec drafts; runtime data, credentials and sessions | Excluded from this change. The supplied source inventory and owner's original checkout remain recovery sources; this ticket does not clean them. |

The complete non-runtime supplied-path inventory and checksums are retained in
[issue-32-inputs.json](issue-32-inputs.json). These are provenance, not claims
that every supplied file belongs in this output. No session/retrospective data
or runtime history has been committed. Do not infer telemetry improvements from
this preservation task.

## Preservation and verification

Both writable task worktrees remain on `sandcastle/issue-32-host-validation` and are
committed independently. The original host checkouts were not available for
mutation and their recorded staged/unstaged/untracked work is left alone.
Editor origin remains `https://github.com/AAAiee/GameFramework.git`. The fork
checkout remains at `/home/agent/projects/fork`, retaining fork origin
`https://github.com/AAAiee/sandcastleCustomized.git` and upstream
`https://github.com/mattpocock/sandcastle.git`.

Local `refs/sandcastle/recovery/issue-32-input` refs retain each supplied input.
The task container's initial fork CRLF-only working-tree drift was saved to
`/home/agent/issue-32-recovery/fork-start.patch` before normalizing only those
byte differences to HEAD. Initial status and remote inventories are alongside it.
The normalization was gated on `git diff --ignore-space-at-eol --exit-code`.
No substantive local patch, index entry or untracked file was discarded.
The input refs and fixture preserve recovery data without importing a history of
execution artifacts. Recovery copies and original input commits are not removed
as part of handoff.

The original suite exercises disposable real Git repositories to verify:
staged/unstaged/untracked restoration, refusal on untracked collisions,
restoration conflicts retaining the recovery stash/checkpoint, separate-worktree
enforcement and ignored build-cache reuse. It also verifies review/candidate
matching, exhaustion, resumption and lost GitHub response reconciliation with
injected commands. Fork tests rerun the 29 reusable cases plus two new boundary
checks for explicit publication text and opt-in close-label removal.

Run the original suite with Node 22+ and Git, without installing dependencies:

```sh
npm run test:sandcastle-baseline
```

It is declared in `Tests/acceptance.json` as an additional Node check. Existing
Regression selectors, Windows checks, submodule pins and native implementations
are unchanged. In the fork, run `npm run build`, `npm run test:workflow` and
`npm run typecheck` using the host's prepared dependencies. Build/typecheck could
not run in this Linux container because `tsup` and `tsgo` are not installed;
workflow tests require only Node/Git and did run. Dependency downloads were not
attempted. Host reports must supply the build/typecheck and native results.

## Handoff to extraction

The reusable fork baseline is versioned at
`1cd36c8d15c60ec6fa19071260ca202ac9de5aad` in sandcastleCustomized.
This identifies the preserved source; current host validation outcomes remain
in host reports.

Review the two local repository commits together, then create PRs manually in
their owning repositories. No push, PR, merge, comment, label change, issue
closure, Editor delivery or enqueueing was performed. No task was added to the
old spec-11 runner. Its active entrypoints and configuration were not changed.

Next establish portable adapters at the public workflow boundary and prove
behavioral parity with this baseline before changing recovery budgets, evidence
identity, diagnostics, context or measurement. Preserve all original acceptance
gates. Native acceptance and Editor context remain consumer-owned. Runtime data
and credentials remain outside the fork. Pinned reproducible consumption and an
independent consumer test are subsequent tickets, not claims made by this change.
