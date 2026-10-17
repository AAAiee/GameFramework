# Codebase audit: bounded corrections and remaining work

This publication incorporates `/opt/editor-local-docs/codebase-improvement-plan.md` (2026-10-14 draft) and its companion guide. Draft claims of a 139-file scan and the original failed Debug baseline describe that earlier audit, not a new exhaustive runtime review. The draft recorded baseline C2607 at primary Scene hooks and a two-line F01 Debug experiment; those are historical reports. Its statements that F01 was uncommitted, no Regression target existed, and all fixes were merely proposals are superseded by the accepted base.

Scope: [parent #11 specification](https://github.com/AAAiee/GameFramework/issues/11#issuecomment-6151488433). This is documentation/ticket preparation, not authorization to implement or schedule the exclusions below. The Windows host owns testing, merges, issue labels/comments/closure. No new ADR or glossary entry is warranted by this contract review.

## Bounded correction inventory

Static inspection of the accepted base and supplied runtime evidence are separate. [The evidence ledger](validation/spec-11.md) records commit, exact commands, outcomes and pending gates; a code row here alone is not an observed pass.

| Finding / issue | Current bounded behavior inspected | Production regression boundary |
| --- | --- | --- |
| F01 / #12 | Dependent primary hook failures; supported explicit hooks preserved; genuine editor fixture | `scene-hooks`, `editor-issue-12` |
| F02 / #13 | Owned inverse projection snapshot | `camera-inverse` |
| F03 / #14 | Capacity at both append sites; slot reuse and empty flush | `renderer-batches` |
| F04 / #15 | Wide-character capacity; new Save As, overwrite/cancel | `editor-issue-15` native dialogs |
| F05 / #16 | Literal labels and linked ImGui string adapter | `entity-tags`, `editor-issue-16` |
| F06 / #17 | Explicit ImVec2, integer usable pixels, restored style | `viewport-pixels`, `editor-issue-17` |
| F07 / #18 | Positive restore clears minimized state; discarded minimized lag | `minimize-restore`, `editor-issue-18` |
| F08 / #19 | Partition-local absent pop; ownership transfer documented | `layer-partition` |
| F09 / #20 | Correct released identity and instantiated immediate API | `typed-events` |
| F10 / #21 | Exact bounded handler-name copies | `event-names` |
| F11 / #22 | Atlas pointer registration, exact lookup, explicit font failure | `font-atlas` |
| F12 / #23 | Missing atlas image retains its declared frame slot | `atlas-placeholders` |
| F13 / #24 | Owned C strings and preserved unpack alignment | `gl-boundaries` |
| F14 / #24 | Repeatable GL headers; pointer assertion before dereference | `gl-buffers` |

[The reading guide](codebase-guide.md) maps the module contracts and documents actual serialization, borrowed entity identity, installed versus dormant systems, and context lifetimes. #25 publishes comments and documentation; its repair also extends the existing real ImGui panel fixture with Inspector menu and checkbox checks. No source-inclusion change requires project regeneration locally. The ledger records the successful combined Windows run at repair commit `5cfa6a3613281049e4f20dbaab238bddbb678f99`, including Inspector input in both builds. This later documentation reconciliation changes no code or tests; human interactive observations remain unrun, and the host owns subsequent validation.

## Excluded findings retained from the audit

Line references below are historical draft audit locations; follow the named symbols in current code. These findings are actionable but should not be folded into a local style/comment pass. They remain part of the audit, not claims of completed fixes.

| Area | Evidence and consequence | Decision needed |
| --- | --- | --- |
| Independent scene viewports | `WindowSystem.cpp:18,28` uses the same subscriber name for every scene; queued viewport events have no scene identity. Two simultaneous scenes cannot follow independent viewport sizes. | Prefer direct scene-local camera updates, or introduce scene-addressed events with a deliberate lifetime contract. |
| Scene load/save failure | `SceneSerializer.cpp:189–228` does not contain YAML parse/conversion failures and commits entities incrementally. `EditorLayer::openScene()` destroys the old scene first and ignores the result. File writes do not report final stream failures. | Parse/validate before commit and preserve the current scene on failure; settle viewport ownership before staging a second scene. Confirm malformed camera/range policy and write-error reporting. |
| Sprite sorting | `RenderSystem.cpp:49–55` fetches Transform for every Sprite, while the editor permits removing Transform separately. | Sort only drawable entities, or establish/enforce a sprite-requires-transform invariant. Verify actual depth iteration order. |
| Teardown and successive applications | `LayerStack` deletes without detach; Application stops the renderer before destroying layer resources, leaves `s_Instance`, and retains captured callbacks through process-lived event storage. Renderer state cannot initialize again after shutdown. Fallback texture caching can outlive its GL context. | Define attachment-aware detach/destruction order and service owners together. A separate engine-runtime owner is a design option, not an existing class. |
| Handler/callback mutation | EventBus and several scene systems iterate storage that callbacks can mutate. Animation callbacks capture relocatable component references. | Explicitly prohibit mutation during dispatch, or defer/snapshot it; capture stable registry/entity identity for animation callbacks. |
| Entity validity | Entity truthiness does not check registry validity; scene removal does not reject a foreign-scene wrapper. | Define borrowed wrapper validity and fail-fast ownership checks. It must still not outlive its scene. |
| Native scripts | Owning raw instance pointers leak on component removal and can shallow-copy; shutdown does not invoke `onDestory()`. Factory output is used before validation. | Define exclusive component ownership and lifecycle hook order. |
| Dormant input/animation/collision systems | Input inverse matrix order differs from rendering, and perspective unprojection lacks homogeneous division/plane definition. Nonlooping animation completion repeats. Collision detection disagrees with drawn offset centers and requires optional debug components. | Settle supported semantics before enabling these systems; fix their small algebra/storage defects within that agreed scope. |
| Legacy renderer paths | Default indexed count zero makes `Renderer::submit()` draw nothing; legacy orthographic VP order differs from the active renderer. Vertex arrays do not correctly support advertised integer/matrix/multiple-buffer layouts. | Explicit counts versus zero-as-default; restrict supported layouts or implement the advertised capabilities. |
| Failure behavior in Release | Shader failure cleanup continues through invalid/deleted handles; asset lookup can insert empty entries after Debug assertions disappear; texture failure handling relies on Debug assertions (named font loading now throws explicitly). | Define always-active invariant failures and recoverable external-input failures consistently. Avoid adding broad catch-all handlers or silent fallback checks. |
| Standards and build policy | The draft identified conflicting build/format guidance; this task explicitly preserves C++20/Premake/MSVC and existing public names. | Keep the existing C++20/Premake build for this pass; decide any CMake migration or whole-repository warning/format enforcement separately. No coding standard was edited. |

Additional smaller candidates: dormant profiler names borrow strings; animation setter `setRunOnce` writes a loop flag; timer services at most one interval per update and accepts unusable durations; the scene renderer allocates and discards an unused framebuffer through a static factory call. The latter is wasteful, not a null dereference. Confirm intended contracts before removing or renaming APIs.
