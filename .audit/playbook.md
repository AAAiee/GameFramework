# Playbook: RB baseline test layering

Rigor: high. New tests become CI gates. Experiments decide CI matrix and whether smokes call a path API.

## Done predicate

1. `UnitTests.exe` runs without GPU or window and covers today-testable Engine and Editor logic (camera math, events, LayerStack, serializer fields if constructible, component data the Inspector already edits).
2. One engine CLI smoke and one editor CLI smoke cover the live main path. Oracle is exit code, process completion, and that a `.rubber` file round-trips at file level.
3. GitHub `windows-latest` runs full UnitTests in Debug and Release, plus only the smokes that experiment showed survive that runner.
4. The same assertion is not owned by both layers.
5. Existing F01-F14 / issue #12-#25 checks are classified so we do not clone their oracles.

## Sequence

1. Inventory existing selectors, Windows scripts, UnitTests.
2. Map modules from call and link edges. List existing user-facing features. Mark GL / ImGui / native dialog / singleton locks. Confirm which systems Scene actually installs.
3. Experiment A: build and run UnitTests and at least one Regression selector on hosted windows-latest (or a no-hardware-GL stand-in). Record survivors.
4. Experiment B: drive New, createEntity, Transform, Save/Load without `onImGuiRender`. Count call sites if a path facade is required.
5. Architect the UnitTests source list and smoke entry shape from those results.
6. Land baseline doctest cases. Characterization of current behavior, including known identity placeholder `123123141`.
7. Land two thin smokes.
8. Land workflow.
9. Break a pure-logic unit (smoke stays green). Break Save (units stay green). Revert.
10. Independent review.

## Non-goals

No F01-F14 coverage program. No per-cpp test-name list as the deliverable. No copy of Regression oracles into doctest. No Application / Renderer / ImGui rewrite for testability. No tests of Input/Animation/Collision unless Scene installs them. No writes under the dirty main checkout.
