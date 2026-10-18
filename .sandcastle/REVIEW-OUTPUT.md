# Independent review output

Set `reviewOutput: true` on a new `runEditorTask(options)` to enable the fork's
strict extraction and one persisted format-correction policy. This also enables
the durable recovery ledger. Existing callers that omit it retain their earlier
fail-fast parsed-review behavior. Once saved, the strict policy stays enabled on
ordinary resume even if the caller omits the flag.

The original `@ai-hero/sandcastle@0.12.0` still invokes both independent reviewer
operations through `sandbox.run`, with one iteration each. The adapter prompts
for final strict JSON containing `task` (exact `task.id`), `candidate`, `approved`
and every criterion exactly once with meaningful evidence. It returns raw stdout
to the customized workflow; no unexported SDK parser or per-call output retry is
used. Tagged, fenced or complete JSON output is extracted deterministically.

Malformed output spends one logical correction per task/candidate/implementation
and evidence generation. Consumption and original output/feedback are saved
before invocation. The correction is another independent reviewer invocation
with the same candidate and validation, explicitly preserving actual rejection
and forbidding implementation changes. It has its own three-attempt transport
budget. Only independently valid approval authorizes local handoff. Valid
negative reviews request code repair instead of repeated formatting. Unknown
provider failures stop; the adapter does not classify message substrings as
transient failures.

`review` and `review-correction` measurements record invocation timing and
available original SDK usage snapshots. A successful raw invocation can still
produce malformed JSON; extraction feedback/consumption remains in the workflow
checkpoint. These are not native acceptance or cumulative live-model totals.

The focused integration uses original SDK public transport seams, real Git and a
host `node --test` assertion. It proves correction preserves implementation and
host validation, and reload does not invoke review/correction again after ready.
No model, native Editor run, remote publication or queue execution is implied.
