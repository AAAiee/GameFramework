# RB baseline test layering

Goal: land the headless doctest layer, two GL smokes, and the CI proof that UnitTests covers the engine and ignores the editor layer.

- [ ] 1. Scene::createWithoutSystems + publishViewportResize via EventManager::create()
- [ ] 2. UnitTests links Engine; new unit doctest files (SceneCamera, LayerStack, EventManager, TransformComponent, Scene/Entity, SceneSerializer)
- [ ] 3. EditorLayer saveSceneTo/openSceneFrom; EngineSmoke + EditorSmoke premake targets; run-smokes.ps1
- [ ] 4. baseline-tests.yml (unit required, smokes compile-only, prove-layering); prove-layering.ps1; docs/testing/baseline-layering.md; Tests/README.md how-to
- [ ] Final: status clean, log/branch/HEAD report
