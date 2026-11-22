# CLI New/Save/Load without ImGui (source measurement)

Compiled run is pending hosted CI and a machine with MSVC. This machine has clang without a C++ standard library and no MSBuild.

## Existing API

EditorLayer::newScene (EditorLayer.cpp:198) creates Scene::create, sets the hierarchy context, and calls initializeSceneViewport. It does not call onImGuiRender. It does require a live framebuffer (initializeSceneViewport reads FrameBuffer::getSpecification).

EditorLayer::openScene (206) and saveScene (220) always call fileDialog::openFile / saveFile, then SceneSerializer with the returned path. There is no path-taking public method.

Scene::createEntity, Entity::addComponent<TransformComponent>, and SceneSerializer::serialize/deserialize already take data or filesystem paths. SceneSerializer does not use ImGui.

Scene::create always runs systemsInit (Scene.cpp:42). WindowSystem::init calls Application::getEventManager (WindowSystem.cpp:16). RendererSystem::init calls Application::getWindow and FrameBuffer::create (RenderSystem.cpp:127). Application construction creates a GLFW window and Renderer::init (Application.cpp:39).

## Facade count if we keep EditorLayer as the smoke driver

Add `saveSceneTo(path)` and `openSceneFrom(path)` that contain the serialize/deserialize bodies. Change saveScene/openScene to call fileDialog then those methods.

Edits: EditorLayer.h two declarations, EditorLayer.cpp two methods plus two one-line wrappers. About 20 lines. No ImGui change. Native dialog stays on the menu path.

Smoke still needs Application + GL because newScene -> Scene::create -> RendererSystem::init.

## Facade count if smoke skips EditorLayer

A console target that links Engine, calls Logger::Init, constructs Application, Scene::create, createEntity, add/remove Tag/Transform/Sprite/Visibility/Camera, SceneSerializer serialize then deserialize, exits. Zero EditorLayer edits. Viewport contract is not exercised (no framebuffer panel size). Primary camera is a CameraComponent flag, which this path can set.

That is a command-layer smoke, not a click-the-panel smoke. ImGui robot is not required for New/entity/Save/Load data. It is required for Inspector widget edits and hierarchy clicks.

## Recommendation pending CI

Prefer the Engine-linked command smoke for editor data path (new scene, entity lifecycle, yaml save/load). Leave Inspector clicks on the existing host Regression fixtures. Do not add an ImGui robot. Do not add a software rasterizer.

If hosted CI cannot construct Application, keep that smoke off windows-latest and run it on a display/GL host. UnitTests stay on hosted CI.
