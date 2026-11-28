# Baseline test layering

Why the test suite is split into a headless doctest executable and two
display-host smokes, what each layer covers, and what it deliberately does not.
The audit trail behind this split lives in `.audit/` in this repository:
[classification.md](../../.audit/classification.md),
[experiments.md](../../.audit/experiments.md),
and the `map-*.md` module maps. The F01-F14 oracle table that prevents
duplication is also summarized in [Tests/README.md](../../Tests/README.md).

## Layers

| Layer | Kind | Where it runs | What it proves |
| --- | --- | --- | --- |
| `UnitTests` | doctest console app, links `Engine`, no window | hosted `windows-latest`, Debug and Release | SceneCamera math, LayerStack ordering/ownership, EventManager queues and typed dispatch, TransformComponent matrices, Scene/Entity registry behavior on a no-systems scene, SceneSerializer roundtrip |
| `EngineSmoke` | console app, links `Engine`, constructs `Application` | display/GL host only (`Tests/Windows/run-smokes.ps1`) | A real Application plus a system-installed Scene survives construction and a few fixed updates; oracle is the exit code |
| `EditorSmoke` | console app, links `Engine`, constructs `Application` | display/GL host only (`Tests/Windows/run-smokes.ps1`) | The command-layer New/entity/Transform/Save/Load path writes a nonempty `.rubber` and deserializes it back; oracle is the exit code |

Hosted `windows-latest` has no WGL/OpenGL driver (measured:
[regression-results.tsv](../../.audit/hosted-ci-probe/regression-results.tsv),
`gl-buffers` exit `-2147483645`, run
[45023232349](https://github.com/AAAiee/GameFramework/actions/runs/45023232349)).
Anything that constructs `Application` dies there, so hosted CI runs UnitTests,
the UI catalog scan, and the layering proof. EngineSmoke and EditorSmoke run on
a display host through `Tests/Windows/run-smokes.ps1`. Editor acceptance on that
host is `verify-rb-editor`.

## Production seams the tests rely on

- `Scene::createWithoutSystems()` constructs a scene with an empty system
  list. Registry work, component hooks, and serialization need no
  `Application`, window, or GL context. `Scene::create()` still installs
  `WindowSystem` then `RendererSystem`; the default runtime path is unchanged.
- `Scene::publishViewportResize` enqueues through the `EventManager` process
  singleton instead of `Application::getEventManager()`. A live Application
  already holds that same singleton, so camera hooks work with or without one.
- `EditorLayer::saveSceneTo(path)` / `openSceneFrom(path)` hold the
  dialog-free bodies; `saveScene` / `openScene` remain thin wrappers around
  the native file dialogs. No ImGui path was added.

## Why EditorSmoke skips EditorLayer

Driving the real `EditorLayer` requires an attached `ImGuiLayer`, font assets,
and a GL framebuffer before `onAttach` will run. That is more than a small
attach for a data-path smoke, so EditorSmoke drives the same bodies directly:
`Scene::create` for New, `Scene::createEntity` plus
`addComponent<TransformComponent>` for the entity, and
`SceneSerializer::serialize`/`deserialize` with an explicit path for Save and
Load. The dialog-free editor methods remain the seam for future editor-level
drivers; the host Regression fixtures keep covering real dialogs.

## CI matrix (baseline-tests)

| Job | Runs-on | Work |
| --- | --- | --- |
| `ui-catalog` | `windows-latest` | `Tests/Windows/scan-ui-catalog.ps1`; no MSBuild |
| `unittests` | `windows-latest` | build and run `UnitTests` in Debug and Release via `Tests/Windows/hosted-ci-probe.ps1 -Stage unit`; exit code is the oracle |
| `layering-proof` | `windows-latest` | `Tests/Windows/prove-layering.ps1` |

The layering proof patches, rebuilds, runs, and restores:

1. `SceneCamera::recalculateMatrix` forced to identity: UnitTests must exit
   nonzero, proving the suite covers engine camera math.
2. `EditorLayer::saveSceneTo` with the serialize call skipped: UnitTests must
   exit zero, proving the engine-level suite does not depend on editor code or
   the editor save path. `SceneSerializer` itself is never patched.

Both files are restored with `git checkout --`, including on failure, and the
script fails if the tree is left dirty. The former Regression selector job is
gone from this workflow: hosted `windows-latest` cannot run anything that
constructs `Application`, so a GL selector there can only fail.

## Known limits, deliberately not covered

- Teardown and lifecycle redesign: destroying/restarting an `Application`,
  late layer pushes, and `onDetach` contracts.
- Multi-scene and multi-viewport behavior: `WindowSystem` uses one shared
  subscription name; simultaneous scenes are unsupported.
- Dormant systems: `InputSystem`, `AnimaitonSystem`, `CollisionSystem`,
  `MoveSystem`, `ScriptSystem`, `FsmSystem`, `FsmPostMovingSystem`,
  `CameraSystem` are never installed by a Scene. The `Entity` API rejects
  their components at compile time, so they are not instantiated in tests.
- GPU rendering output: draw counts, batches, texture slots, and atlas
  contents stay on the host Regression suite.
- Native file dialogs and the private-desktop UIAutomation driver.
- Inspector and hierarchy interaction: ImGui widget edits, tag input, menu
  clicks. No click API exists and none was added.
- Keyboard shortcuts: `shortcutCallBack` polls real window key state, so
  synthetic events alone cannot drive Ctrl+N/O/S.
