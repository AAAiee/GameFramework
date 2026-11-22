# RB baseline test layering

Goal: land a baseline suite for existing user-facing Engine + Editor behavior. UnitTests (no GPU, no window) plus two thin CLI smokes, plus CI for what a hosted windows-latest runner can actually run.

- [x] Frame playbook and open a clean worktree from origin/Editor
- [ ] Inventory existing Tests/ checks (dedupe, not coverage target)
- [ ] Testability map by real module + existing features
- [ ] Experiment: no-GPU compile/run of UnitTests and at least one Regression selector
- [ ] Experiment: CLI New / createEntity / Transform / Save-Load without ImGui
- [ ] Architect the test-layer shape from those observations
- [ ] Land baseline UnitTests
- [ ] Land engine + editor smoke CLIs
- [ ] Land CI for UnitTests Debug+Release and proven-alive smokes
- [ ] Prove layering with two intentional breaks
- [ ] Write classification table, map, experiment log, uncovered risks
- [ ] Independent review of trail + diff
