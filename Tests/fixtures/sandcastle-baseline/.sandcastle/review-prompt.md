# Assigned review

{{TASK_CONTEXT}}

Review branch `{{BRANCH}}` against accepted base `{{BASE}}` for this issue only.
The Windows host owns all tracker writes, merging, and final acceptance.

Previous failure, if any:
{{FEEDBACK}}

# Review process

Current Windows host evidence (for the candidate SHA):
{{VALIDATION}}

Reports are mounted at `/opt/editor-validation`; use the directory basename from
the report path to locate detailed logs. Earlier failure feedback is historical;
evaluate runtime results using the current matching-SHA evidence above.
The user requested unattended validation. Assess native behavior using actual
dialog assertions and Windows logs; human observations are recorded separately
as unrun when no human performed them. Identify any behavior still uncovered by
automation rather than requesting manual repetition of demonstrated coverage.
Repository documentation can preserve a completed checkpoint's SHA and observed
results while linking the final acceptance report and issue publication. A Git
commit cannot embed its own hash in its contents: changing the hash text creates
a different commit. Require the final candidate's full SHA and actual outcomes
in the external host report supplied above (and its host-owned tracker record),
not a self-reference in committed Markdown. Check that checkpoint claims match
their named evidence, final-report pointers resolve, and final evidence matches
HEAD. Do not interpret a procedural description of report generation as an
unrun check when that exact report is supplied and passed. This does not permit
using an earlier checkpoint in place of current-candidate Windows validation.

1. Inspect `git diff {{BASE}}...HEAD` and the complete acceptance criteria.
   Apply `.sandcastle/CODING_STANDARDS.md` for code style and scope; the current
   host workflow supersedes its historical manual validation handoff wording.
2. Check correctness, scope, documentation, and test oracles. Verify that each
   configured selector exercises the actual affected production behavior, stays
   active in Release, and preserves the complete-suite invocation. Inspect each
   configured Windows UI script: it must drive real behavior, assert outcomes,
   fail on errors/timeouts, and emit the marker only on success. Source inspection
   or an unconditional PASS cannot substitute for editor/dialog observations.
3. Review read-only. Report necessary corrections in rejected criteria so the
   implementer can fix them and the host can validate the new commit.
4. Get the final HEAD SHA. Account for every supplied criterion exactly once in
   the result below. Evidence names the relevant source/test and the host check
   that established runtime behavior. Approval requires passing host evidence
   for this exact SHA and adequate assertions for the criteria. If a criterion
   lacks a credible check or implementation, set approved/satisfied to false.

# Result

Emit one JSON object in `<review>` tags. Use the actual issue number, final SHA,
and all C1..Cn identifiers supplied in TASK_CONTEXT (not a subset):

```text
<review>{"issue":12,"sha":"<final HEAD>","approved":true,"criteria":[{"id":"C1","satisfied":true,"evidence":"Specific source location, assertion, and matching-SHA Windows result."}]}</review>
```

After the result, emit <promise>COMPLETE</promise>.
