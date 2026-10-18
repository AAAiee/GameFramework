# Measurements and extracted baseline: #36

The task adapter uses the fork's opt-in `workflow/measurements` public interface.
Run-scoped `records/measurements.json` contains a durable start and final record
for each implementation, host validation, review and local handoff invocation.
Records carry project, run, task and environment identity, elapsed host time,
attempt/outcome and available usage. Reload preserves earlier attempts; an
unfinished measurement is explicitly interrupted, with unknown elapsed time.
The collector does not decide whether an interrupted operation can be replayed.

The integration package update selects committed fork revision
`173da157d4673f9894102b3da4347e120899c1af`. Its offline Linux build produced archive
SHA-256 `4f1e0943250d583f336f391aa173a29fa1179eeba6d4e06a40d9a015d62e7362`;
[package-provenance.json](package-provenance.json) records the exact source,
toolchain and build inputs. The tracked archive, dependency and lockfile are
updated together. One pinned build was performed for this increment; independent
final-artifact reproduction remains a #43 gate. The preserved timing baseline
below retains its original Windows development-build identity and is not relabeled
as a measurement of this integration archive.

The original SDK exposes usage on `result.iterations.at(-1).usage`. Its documented
last-message snapshot is preserved as `observedUsage`, while cumulative stage token
totals remain `null`. Codex's SDK input field already excludes cached tokens; the
normalization adds fresh input, cache creation and cache reads exactly once. A
missing provider metric or retry count remains unavailable, rather than zero.
Durable recovery counters, when supplied by the workflow, feed the measurement's
attempt/retry metadata; instrumentation does not create retry policy.

The host validation callback receives `measureCheck(name, action)`. Use it for
each actual native command/check. For example:

```js
validate: async ({ worktree, candidate, measureCheck }) => {
  const result = await measureCheck("configured-check-name", () =>
    runConfiguredCheck({ worktree, candidate }),
  );
  return { candidate, passed: result.passed, evidence: result.evidence };
};
```

`runConfiguredCheck` above is a consumer operation, not a new fork API. Check
results with `passed: false` are recorded as rejected; exceptions are failed. The
adapter includes check timing/outcome records in `validation.timings`, so the host
handoff and independent reviewer can see them. Legacy opaque validators still get
whole-stage timing; their individual command timing is unavailable until they use
this callback. Native diagnostic/acceptance integration must retain per-command
coverage when introduced. Parent stages and child checks overlap, so their times
must not be summed as end-to-end wall time.

[measurement-baseline.json](measurement-baseline.json) is an actual host-clock
observation of the extracted workflow before recovery/context/diagnostic
optimizations. Scripted agents create a real Git commit, a host Node assertion
executes, an independent scripted review approves, and delivery creates a local
handoff. It records task/candidate, platform, Node version and the SHA-256 of the
actual installed measurement module. The fixture compatibility identity is scoped
to the test and is not an Editor native toolchain identity. The test can recapture
an explicitly named observation with `SANDCASTLE_BASELINE_OUTPUT` set to a file;
normal tests never rewrite the preserved baseline.

No model credentials, live model request or native Editor acceptance is involved.
This single fixture establishes host timing/accounting behavior, not production
token usage, native performance or optimization savings. Deterministic fake-clock
tests separately check arithmetic. Initial model selection remains caller-owned:
the preserved live configuration used `gpt-6.1-sol`/medium for implementation and
`gpt-6-luna`/high for review, with a 12-hour run bound. This increment changes none
of those choices and adds no token ceiling. The old runner remains paused.

## Historical comparison context

The 2026-10-17 retrospective reported 53 captured sessions, 50 with usage:
25,364,703 cumulative input tokens, including 23,041,536 cached input tokens, and
272,880 output tokens. Its 44 issue-specific stages had 106.51 minutes of captured
agent-session time. It also inventoried 26 Windows reports and 466 checks, but did
not reconstruct complete native/check/end-to-end timings. Aggregate source:
`.scratch/sandcastle-retro/metrics.json`, SHA-256
`aa9fcb598f86503d2e464b3b36836d4f95c0e255e0751e5d4730ddb919bf68d6`;
preserved separately with the implementation evidence.

The historical tasks, native environment and cumulative session counters differ
from this scripted fixture and the SDK's last-message snapshots. No percentage,
cost, live/peak context size or end-to-end improvement can be inferred from their
comparison. Preserve this baseline before collecting later optimization results;
compare like tasks/environments and disclose missing measurements explicitly.
