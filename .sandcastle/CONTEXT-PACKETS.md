# Complete stage context

The adapter requires explicit `context` alongside the caller's implement/review
prompts. Project instructions remain here; the fork's public `createWorkflowPacket`
owns the reusable stage policy.

```js
context: {
  scope: "Full requested scope, without a second acceptance-criteria copy.",
  base: baseSha, // immutable 40-character Git commit chosen before implementation
  entrypoints: ["Tests/unit/UnitTests.cpp"],
  commands: ["pwsh Tests/Windows/unit-tests.ps1 -Configuration Debug"],
  references: ["Tests/README.md"],
}
```

The task supplies the complete criteria once. Initial implementation gets the
scope, criteria, entrypoints and commands. Repair gets only current failed checks,
concise diagnostics and exact raw evidence locations, plus the full requirements
packet by reference. Review gets every criterion, base/candidate, every changed
path, a full `git diff <base> <candidate> --` command and current evidence summary.
Format correction gets the actual format problem, exact malformed output by
reference and required contract. It must preserve the actual findings and decision.

`.sandcastle/context/<runtime.key>/` must be ignored by the task checkout. The
adapter checks this before writing; it does not modify global Git excludes.
Referenced source/document files and raw host evidence are copied to that mount
with byte-bound names while preserving the original source location. Only files
inside the task checkout/run directory or explicitly listed context references
can be copied. Model-written criterion evidence and repair prose are never
interpreted as filesystem requests. Credential/session files are refused.

Each invocation has unique generation/implementation/transport identity and its
own SDK log, packet and raw output. Copies are also retained under the run's
`stage-context` directory; existing bytes cannot be replaced. Only explicitly
referenced retained evidence is rematerialized on resume. Local handoffs expose
both sandbox-relative and retained host pointers to complete requirements and
the independent review output. This keeps evidence available after SDK cleanup.

Review identity covers the packet-policy version, explicit context, reviewer
prompt and actual current referenced bytes. Changing a command, scope, base,
document path or document contents triggers fresh review while matching host
validation remains eligible. Stable inputs do not create new review work just
because invocation filenames differ. Current budgets and correction consumption
remain owned by the workflow; packet construction cannot reset them.

Tests observe actual original-SDK prompts through the public task adapter, read
referenced files inside its worktree, verify exact failure/correction bytes and
same-candidate context invalidation. The independent installed consumer renders
project A then B without sharing requirements. These are scripted transport
checks, not live model or native acceptance results.
