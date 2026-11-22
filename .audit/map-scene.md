# Testability map: Scene, Entity, components, SceneCamera, SceneSerializer, Systems

Scope: `Engine/Source/Rubber/Scene/` including `System/` and `Utili/Serializer/`, read at HEAD `494c32c4ec085f83e78b7ba21b258f781a25b567`. The feature list is derived from code only. No production file was changed.

## Head verdicts

- **Systems that run in a default Scene: exactly two.** `Scene::systemsInit` installs `WindowSystem` then `RendererSystem` and initializes both immediately (Engine/Source/Rubber/Scene/Scene.cpp:51-59). A repo-wide search over `*.h`/`*.cpp` finds no other instantiation of any `*System` class outside their own definitions and the `Rubber.h` umbrella includes (Engine/Source/Rubber.h:55-64). Corroborated by docs/codebase-guide.md:51-56 and Tests/README.md:414-420.
- **Scene cannot be constructed without GPU.** The constructor is private and calls `systemsInit` (Scene.cpp:42-44); `Scene::create` is the only path (Scene.cpp:83-86). `WindowSystem::init` requires `Application::getEventManager()` (System/WindowSystem.cpp:16), which dereferences the `s_Instance` singleton with no null guard (Engine/Source/Rubber/Core/Application.cpp:97-100). `RendererSystem::init` requires `Application::getWindow()` dimensions (System/RenderSystem.cpp:133-134) and a `FrameBuffer::create` call that needs a live GL context (System/RenderSystem.cpp:136; Engine/Source/Rubber/Renderer/FrameBuffer.h:17,35). Constructing an `Application` itself creates a GLFW window, an `ImGuiLayer`, and calls `Renderer::init()` (Application.cpp:39-84). There is no headless construction path.
- **SceneSerializer cannot run without a Scene**, and a Scene cannot exist headless, so the serializer inherits that lock even though its own code is CPU-only (its constructor only stores a `Ref<Scene>`, SceneSerializer.cpp:150-151).
- **No submodule in this scope needs ImGui or a native dialog.** Those dependencies live in the Editor (`EditorLayer`, `ImGuiLayer`) and are out of scope here.

## System install table

| System | Status in a default Scene | Evidence |
| --- | --- | --- |
| WindowSystem | **Installed.** Subscribes to viewport resize and updates non-fixed-aspect cameras. | Scene.cpp:53; WindowSystem.cpp:12-23 (subscribe name `"Viewport Resize"` at line 18), 33-55 |
| RendererSystem | **Installed.** Sorts sprites by z, draws quads and collision boxes via `Renderer2D`. | Scene.cpp:54; RenderSystem.cpp:21-125 |
| InputSystem | **Dormant.** Never instantiated anywhere. Would need `Application::getEventManager()` and `Application::getWindow()`. | No instantiation site (grep, whole repo); InputSystem.cpp:14-45, 105-107 |
| AnimaitonSystem (typo is the class name) | **Dormant.** Never instantiated. Would need `Application::getAssetManager()` and a registry `on_construct<AnimationComponent>` hook. | No instantiation site; AnimationSystem.h:7, AnimationSystem.cpp:18-22, 74 |
| CollisionSystem | **Dormant.** Never instantiated. Pure AABB math over the registry. | No instantiation site; CollisionSystem.cpp:13-72 |
| MoveSystem | **Dormant.** Never instantiated. Pure velocity/gravity integration. | No instantiation site; MoveSystem.cpp:10-29 |
| ScriptSystem | **Dormant.** Never instantiated. Instantiates and updates `NativeScriptComponent` scripts. | No instantiation site; ScriptSystem.cpp:18-34 |
| FsmSystem | **Dormant.** Never instantiated; it is a template over a state enum, so it cannot be installed by the non-template `systemsInit` without a concrete `StateEnum`. | FsmSystem.h:7-8; no instantiation site |
| FsmPostMovingSystem | **Dormant.** Same template limitation. | FsmPostMoving.h:9-10; no instantiation site |
| CameraSystem | **Dormant.** Never instantiated; its `onUpdate` is a no-op that only asserts the registry pointer. | CameraSystem.cpp:7-10 |

`SystemBase::init` guards double init and requires a non-null scene (System/System.cpp:7-19). `onSystemsUpdate` asserts each system was initialized, in insertion order (Scene.cpp:70-76). Shutdown runs insertion order and clears ownership (Scene.cpp:61-67).

## Submodule detail

### Scene
- User-facing behavior: creates entities with a default tag (empty becomes `"Unknown Entity"`, Scene.cpp:15-24); destroys entities (26-29); stores viewport dimensions for later camera delivery (36-40); queues a viewport resize event for `WindowSystem` (159-164); updates installed systems in insertion order (70-81).
- Flags: **singleton-locked** (Application via `publishViewportResize`, Scene.cpp:161), **needs-GL** (via installed RendererSystem), needs a running `Logger` (the destructor logs, Scene.cpp:47).
- `WindowSystem` shares one subscription name across instances (WindowSystem.cpp:18,28), and `EventBus` stores handlers in per-event-type statics that assert on duplicate names (Engine/Source/Rubber/Event/EventBus.h:37,101-102). Two simultaneous Scenes with independent viewport delivery are therefore not supported, as Scene.h:21-22 documents.
- TUs for a no-Engine UnitTests case: Scene.cpp includes Application.h, EventManager.h, Renderer2D.h, RenderSystem.h, WindowSystem.h (Scene.cpp:3-10), so linking it pulls the Application/Renderer/GLFW chain. Not headless as written.

### Entity
- User-facing behavior: borrowed wrapper over `(entt::entity, Scene*)` (Entity.cpp:8-11); `hasComponent` returns false for invalid handles (Entity.h:20-26); `addComponent` runs the Scene hook then emplaces (Entity.h:32-37); `removeComponent`, `getComponent` follow the same borrow rules (Entity.h:40-54); boolean conversion tests stored values only (Entity.h:65-67).
- Flags: **singleton-locked indirectly**. Adding or removing a `CameraComponent` fires `publishViewportResize` (Scene.cpp:108-111,153-156), which needs the Application singleton. Other component paths are pure registry work.
- TUs: Entity.cpp alone is trivial, but every `addComponent`/`removeComponent` instantiation references the `Scene::onComponent*` specializations defined in Scene.cpp (Entity.h:35,44), which drags the full chain.

### Components
- User-facing behavior: data structs plus Scene-mediated hooks. The supported hook set is exactly `TagComponent`, `SpriteComponent`, `CameraComponent`, `TransformComponent`, `VisibilityControlComponent` (Scene.cpp:96-156). Any other type instantiated through `Entity::addComponent`/`removeComponent` trips `static_assert(!std::is_same_v<T, T>, ...)` (Scene.cpp:89-93,125-128), so `MoveComponent`, `CollisionComponent`, `CollisionBoxOnRenderFlag`, `InputComponent`, `StateMachineComponent`, `NativeScriptComponent`, `AnimationComponent` cannot be added through the Entity API at all today. That is a compile error, not a runtime one.
- Flags: **pure-logic-unit** as headers. Component.h pulls only GL-free headers (Renderer/Camera.h, Renderer/Texture.h, Renderer/Subtexture2D.h, Animation/AnimationSpec.h, ScriptableEntity.h, Input/KeyCodes.h; verified none includes glad/GLFW/ImGui, and Subtexture2D.h:1-3 is glm plus Utility.h only). One CPU trap: `SceneCamera::onResize` divides `width/height` with no zero guard (SceneCamera.cpp:34).
- TUs: header-only; a no-Engine test can construct components and exercise `TransformComponent::getTransformMatrix` (Component.h:32-40) with glm alone.

### SceneCamera
- User-facing behavior: defaults to Ortho, size 10 vertical units, near -1, far 1, aspect 16/9 (SceneCamera.h:66-71); setter calls recalculate immediately; ortho box is `size * aspect` wide and `size` tall (SceneCamera.cpp:42-49); perspective path uses fov/near/far (SceneCamera.cpp:52).
- Flags: **pure-logic-unit**. No Application, no Logger calls, no asserts in SceneCamera.cpp.
- TUs for a no-Engine UnitTests case: `Engine/Source/Rubber/Scene/SceneCamera.cpp` only. It includes `pch.h` (compile-time spdlog/glm header paths) but references no Logger or Application symbols, so nothing beyond glm and include dirs is needed. This is the cleanest candidate for the existing no-Engine `UnitTests` project (Tests/Build-UnitTests.lua:8-12 already links no Engine and already tests `Camera.h` the same way in Tests/src/CameraInverseCheck.cpp).

### SceneSerializer
- User-facing behavior: writes editor YAML (SceneSerializer.cpp:153-176); appends fresh entities on load, returns false only for a missing `"Scene"` key (183-191,199-262); a missing or empty tag loads as `"Unknown Entity"` (Scene.cpp:21); runtime entry points are assertion stubs (178-181,267-271).
- YAML fields actually written per entity (SceneSerializer.cpp:91-134): `Entity` (placeholder), `TagComponent.Tag`, `TransformComponent.Position/Rotation/Scale`, `CameraComponent` with nested `Camera.{ProjectionType, PerspectiveFovY, PerspectiveNearClip, PerspectiveFarClip, OrthographicSize, OrthographicNearClip, OrthographicFarClip}` plus `Primary` and `FixedAspectRatio`, `SpriteComponent.Color`, `VisibilityControl.IsVisible`. Top level is `Scene: Untitled` with an `Entities` sequence (157-161).
- **Known identity placeholder:** every entity serializes as `Entity: 123123141` with a TODO UUID (SceneSerializer.cpp:93); the value is read back and discarded (SceneSerializer.cpp:202). Not persistent identity; loading creates fresh entities.
- Not serialized: sprite textures, NativeScript, Animation, Input bindings, Move, Collision, CollisionBoxOnRenderFlag, StateMachine, and any system state. Deserialize is not transactional and YAML conversion errors can throw (SceneSerializer.h:27-31).
- Flags: **singleton-locked** (input `Ref<Scene>` cannot exist headless; `RB_INFO` calls in deserialize need `Logger::Init()` or a null-logger deref, Log.h:32-36, Log.cpp:39-48).
- TUs for a no-Engine UnitTests case: SceneSerializer.cpp plus yaml-cpp (`YAML_CPP_STATIC_DEFINE`, SceneSerializer.cpp:7-9), Logger, and the whole Scene.cpp chain. Not headless as written.

## Cross-cutting headless blockers

- `Application::getEventManager/getWindow/getAssetManager` all dereference `s_Instance` without a null guard (Application.cpp:92-100,112-119). `EventBus<T>` keeps handlers in per-type static storage that outlives Applications and asserts on duplicate subscriber names (EventBus.h:28-41,101-109).
- Every log macro (`RB_INFO`, `RB_CORE_ASSERT` in message form) dereferences the Logger singleton; calling any before `Logger::Init()` dereferences a null `shared_ptr` (Log.h:32-36, Log.cpp:30-48). `Logger::Init()` itself needs only spdlog, no GPU.
- `SystemBase::init` calls `Scene::getRegistry` (System/System.cpp:18), so even the dormant pure-logic system TUs (MoveSystem.cpp, CollisionSystem.cpp, ScriptSystem.cpp, CameraSystem.cpp) link back into Scene.cpp and its Application/GL chain as written. Header-only `FsmSystem.h` and `FsmPostMoving.h` avoid this only until a `SystemBase` vtable is needed.
- Observed in-passing defects (dormant code, no user hits them today): `RendererSystem::init` calls `create` through a default-null `Ref<FrameBuffer>` and discards the result (RenderSystem.cpp:136); `CollisionSystem::onUpdate` `get<CollisionBoxOnRenderFlag>` on colliders whose view does not require that component (CollisionSystem.cpp:35-36); `AnimaitonSystem` class-name typo (AnimationSystem.h:7); `InputSystem::mousePosToWorldCoord` asserts a primary camera exists (InputSystem.cpp:94). Base already documents these areas (docs/codebase-guide.md:35,56,66); none blocks the map.

## TUs summary for a no-Engine UnitTests target

| Candidate | TUs to add | Extra deps beyond current UnitTests project | Verdict |
| --- | --- | --- | --- |
| SceneCamera | SceneCamera.cpp | include dirs for spdlog headers (via pch.h), glm already present | headless-clean |
| Components, TransformComponent math | none (headers) | glm, entt include | headless-clean |
| FsmSystem, FsmPostMovingSystem | none (headers) | entt, Component.h; needs a `SystemBase` stand-in or protected-member subclass to reach `m_Registry` | headless with one test-local seam |
| MoveSystem, CollisionSystem, ScriptSystem, CameraSystem | system cpp + System.cpp | pulls Scene.cpp, Application, Renderer, GLFW/GL/ImGui | blocked as written |
| Entity, Scene, SceneSerializer | Scene.cpp chain | full Engine | blocked as written |
