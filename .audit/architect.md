# Test-layer shape

Picked after the hosted probe and the Scene/Editor/Resources maps.

## Data shape

One headless process (`UnitTests`) and two GL CLI processes (`EngineSmoke`, `EditorSmoke`). One assertion lives in one process.

UnitTests links Engine. It never constructs `Application`, never calls `glfwInit`, never creates a window. Hosted windows-latest can link opengl32 and still run (measured: UnitTests already runs there; `Application` construction is what dies).

A no-systems Scene factory is the registry/serializer seam:

```
enum class Scene::Systems { Installed, None };
static Ref<Scene> create();                 // Installed, current behavior
static Ref<Scene> createWithoutSystems();   // registry and component hooks only
```

`publishViewportResize` uses `EventManager::create()` so a Camera hook does not dereference `Application::s_Instance`. When an Application exists it already stored that same singleton.

## UnitTests (hosted)

New doctest files, no GPU:

- SceneCamera ortho/persp/onResize math
- LayerStack push/pop/overlay order and ownership
- EventManager current/next queue and immediate dispatch
- TransformComponent::getTransformMatrix
- Scene/Entity create/delete, add/remove/has for Tag, Transform, Sprite, Visibility, Camera
- SceneSerializer roundtrip of the fields actually written, including identity `123123141`

Keep the existing `camera-inverse` case. Do not copy Regression require() lists. Do not test InputSystem, AnimaitonSystem, CollisionSystem, MoveSystem, ScriptSystem, Fsm*. They are not installed (Scene.cpp:53-54).

Do not test AnimationTimer as an enabled gameplay feature. The animation system never runs.

Logger::Init once before cases that log.

## Smokes (display/GL host)

EngineSmoke: Logger::Init, Application, Scene::create(), one entity, a few `onSceneUpdate(1/120)`, exit 0. Oracle is exit code. Not on hosted CI.

EditorSmoke: Logger::Init, Application, attach path as needed, `newScene` or Scene::create, createEntity, write Transform, `saveSceneTo(path)`, `openSceneFrom(path)` or serializer load into a fresh scene, file nonempty, deserialize true, exit 0. Viewport contract stays out of hosted CI. No ImGui robot.

EditorLayer gets `saveSceneTo`/`openSceneFrom`. `saveScene`/`openScene` call the dialog then those methods.

## CI

`windows-latest`: UnitTests Debug and Release only. Drop the Regression selector job from the required workflow. `hosted-ci-probe.ps1` stays for reruns.

Layering proof script on that runner: break SceneCamera math, UnitTests fail, restore; no-op EditorLayer::saveSceneTo, UnitTests pass, restore.

## Rejected

- Linking nothing and compiling Scene.cpp into UnitTests. Scene.cpp references WindowSystem and RendererSystem at link time.
- ImGui click driver.
- Software rasterizer.
- Putting Application smokes on windows-latest. WGL has no OpenGL there.
