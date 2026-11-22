# Editor/src testability map

Goal of the parent run: land a baseline test suite for existing user-facing Engine + Editor behavior. This file maps Editor/src. It lists the real user-facing features, traces each editor operation to its production path, and flags what each surface needs before a test can drive it: `needs-GL`, `needs-ImGui`, `needs-native-dialog`, `singleton-locked` (Application/WindowInput/FontManager statics), `pure-logic-unit`. Inspection only. No production file was changed.

Paths are relative to the worktree root `C:\Users\yhl20\.t3\worktrees\GameFramework-rb-test-layering`. All line numbers verified against `git rev-parse HEAD` at the end of this file.

## User-facing features in Editor/src

| Feature | Production site |
| --- | --- |
| Editor application (layer push) | Editor/src/Editor.cpp:11-20 |
| Dockspace shell + File menu (New, Open..., Save As..., Exit) | Editor/src/EditorLayer.cpp:75-157 |
| Stats panel (DrawCount, QuadCount, IndexCount, VertexCount, Frame Time, FPS) | Editor/src/EditorLayer.cpp:159-167 |
| Scene viewport panel (live framebuffer texture, focus/hover flags) | Editor/src/EditorLayer.cpp:171-182 |
| Ctrl+N / Ctrl+O / Ctrl+S shortcuts | Editor/src/EditorLayer.cpp:229-264 |
| Scene hierarchy list/select/deselect/create/delete | Editor/src/Panels/SceneHierachyPanel.cpp:135-211 |
| Inspector: Tag edit, Add Component menu, Transform XYZ, Visibility, Camera, Sprite color | Editor/src/Panels/SceneHierachyPanel.cpp:213-369 |
| Font theming (OpenSans Regular/Bold) | Editor/src/EditorLayer.cpp:22-36, Editor/src/Utilities/FontsManager/FontManager.cpp:19-62 |
| Scene Save/Open via YAML (.rubber) | Editor/src/EditorLayer.cpp:198-227, Engine/Source/Rubber/Scene/Utili/Serializer/SceneSerializer.cpp:153-265 |
| Native file dialogs | Engine/Source/Platform/Windows/WindowsPlatformUtil.cpp:14-43 |

## Trace of the requested paths

### EditorLayer::onAttach (Editor/src/EditorLayer.cpp:20-50)

Order: `ImGui::GetIO()` (22), `FontManager::loadFontFromTTF` twice (30, 33), `setDefaultFont` (36), window size from `Application::getWindow()` (38-39), `FrameBuffer::create` (41), `Scene::create` (42), `initializeSceneViewport()` (43), panel `setContext` (44), `EventManager::subscribe<KeyPressedEvent>` for shortcuts (47-49). The class doc states it must run after ImGui attachment (Editor/src/EditorLayer.h:18-24, 29).

Requirements: ImGui context (fonts + IO), GL context (`FrameBuffer::create` goes `Renderer::getAPI()` to `GLFrameBuffer`, Engine/Source/Rubber/Renderer/FrameBuffer.cpp:10-23, whose ctor calls `glCreateFramebuffers`/`glCreateTextures`, Engine/Source/Platform/OpenGL/GLFrameBuffer.cpp:8-44), Application singleton (window size at 38-39; `Scene::create` constructs `WindowSystem` + `RendererSystem`, Engine/Source/Rubber/Scene/Scene.cpp:42-59, and `RendererSystem::init` reads `Application::getWindow()` and creates its own GL framebuffer, Engine/Source/Rubber/Scene/System/RenderSystem.cpp:127-137), and the font asset files at `assets/fonts/OpenSans/static/*.ttf` relative to the working directory.

### EditorLayer::newScene (Editor/src/EditorLayer.cpp:198-204)

`m_ActiveScene.reset()` (200, old Scene shutdown unsubscribes via `Application::getEventManager()`, Engine/Source/Rubber/Scene/System/WindowSystem.cpp:25-30), `Scene::create()` (201), panel `setContext` (202), `initializeSceneViewport()` (203).

Does EditorLayer::newScene require framebuffer/GL? Yes, on both counts, though nothing in its body says GL:
1. `initializeSceneViewport` dereferences `m_FrameBuffer->getSpecification()` (Editor/src/EditorLayer.cpp:191). `m_FrameBuffer` is only ever assigned in `onAttach` (41) under a GL context. Calling `newScene` without a prior `onAttach` crashes on a null `Ref`.
2. `Scene::create` constructs `RendererSystem`, whose `init` creates a GL framebuffer and reads the Application window (Engine/Source/Rubber/Scene/System/RenderSystem.cpp:131-137). A current GL context and the Application singleton are hard prerequisites.

It makes no ImGui call. It is reachable without an active ImGui frame, but only through the friend fixtures (below) or the Ctrl+N shortcut.

### EditorLayer::openScene (Editor/src/EditorLayer.cpp:206-218)

`fileDialog::openFile` (208), then on a nonempty path: scene reset/recreate, panel `setContext`, `initializeSceneViewport` (210-213), `SceneSerializer::deserialize` (215-216). The dialog is modal on the Application window HWND and must run on the UI thread (Engine/Source/Platform/Windows/WindowsPlatformUtil.cpp:19; the fixture asserts the UI thread, Tests/src/EditorIssue15Fixture.cpp:523-529). The body after the dialog is ImGui-free.

### EditorLayer::saveScene (Editor/src/EditorLayer.cpp:220-227)

`fileDialog::saveFile` (222), then `SceneSerializer::serialize` on a nonempty path (224-225). No ImGui, no GL calls; the serializer writes YAML to disk (Engine/Source/Rubber/Scene/Utili/Serializer/SceneSerializer.cpp:153-176, 136-146).

### EditorLayer::shortcutCallBack (Editor/src/EditorLayer.cpp:229-264)

Repeat events bail (231). `Input::isKeyPressed(RB_KEY_LEFT_CONTROL/RIGHT_CONTROL)` (234) goes through `WindowInput::isKeyPressedImpl`, which polls `glfwGetKey` on `Application::getWindow().getNativeWindow()` (Engine/Source/Platform/Windows/WindowInput.cpp:10-14). The switch dispatches Ctrl+O (237-244), Ctrl+N (245-252), Ctrl+S (254-261). A `KeyPressedEvent` can be enqueued and dispatched synthetically through the EventManager, but the modifier check reads real window key state, so a synthetic event alone never triggers the branch. The shortcut path is therefore not drivable without a real window with real Ctrl input state. This is an honest dead end for headless driving, not something a fixture can fake through the public API.

### EditorLayer::initializeSceneViewport (Editor/src/EditorLayer.cpp:188-196)

Uses last usable sampled pixels, else falls back to the framebuffer spec (191-193), then `m_ActiveScene->updatesViewportSize` (194-195). Called from `onAttach` (43), `newScene` (203), `openScene` (213). Requires a non-null `m_FrameBuffer` (GL-created) and a live Scene.

### Viewport contract

`onImGuiRender` records the next size: `ImGui::GetContentRegionAvail` into `m_ViewPortDimension` (Editor/src/EditorLayer.cpp:178-179). `onUpdate` consumes the preceding size: quantize via the anonymous-namespace `viewportPixels` (57-66; helper at 8-14 rejects non-finite, <1.0, or >uint32 max, then truncates toward zero, 800.5 to 800), resize the framebuffer if the integer pixels changed, `enqueue(ViewPortResizeEvent)`, `Scene::updatesViewportSize` (62-66). Rendering happens into the framebuffer each fixed update (69-72). `Scene` stores the dimensions for camera construction hooks (Engine/Source/Rubber/Scene/Scene.cpp:36-40); adding or removing a `CameraComponent` publishes `ViewPortResizeEvent` through `Application::getEventManager()` (Engine/Source/Rubber/Scene/Scene.cpp:107-111, 153-156, 159-164), and `WindowSystem` delivers to non-fixed-aspect cameras (Engine/Source/Rubber/Scene/System/WindowSystem.cpp:33-55). Independent simultaneous viewports are unsupported (Editor/src/EditorLayer.h:23; shared subscription name, Engine/Source/Rubber/Scene/Scene.h:21-22).

The quantization helper `viewportPixels` (Editor/src/EditorLayer.cpp:8-14) is pure logic but lives in an anonymous namespace inside the .cpp. No test can link it directly. It is reachable only through `onUpdate` and `initializeSceneViewport`, which is how Tests/src/ViewportPixelsCheck.cpp:60-106 drives it.

### Inspector drawComponent list (SceneHierachyPanel.cpp:213-369)

drawComponentNode draws, in order:
- TagComponent "Tag", `InputText("##Tag")` via imgui_stdlib, not removable (216-220).
- "Add Component" button + popup (225-271): Camera, Sprite Renderer, Transform, Visibility menu items, each guarded by `hasComponent`.
- TransformComponent "Transform", removable (275-285): `drawVec3Control` Position, Rotation (degrees display, radians storage), Scale. `drawVec3Control` (11-73) uses `FontManager::getFont("OpenSans-Bold")` (35, 49, 59).
- VisibilityControlComponent "Visibility", removable (287-290): `Checkbox("isVisible")`.
- CameraComponent "Camera", removable (293-361): Primary and IsFixedRatio checkboxes, Projection combo, Ortho (Size/NearClip/FarClip DragFloats, 323-339), Persp (FOVY/NearClip/FarClip, 342-359).
- SpriteComponent "Sprite Renderer", removable (365-368): `ColorEdit4`.

### Hierarchy create/delete/select (SceneHierachyPanel.cpp:135-211)

onImGuiRender lists every registry entity (140-145), blank-space mousedown deselects (148-150), right-click blank opens "Create Entity" which calls `ContextRef->createEntity("Empty Entity")` (153-157). drawEntityNode selects on item click (184-186), right-click opens "Delete Entity" (191-195), removal calls `Scene::removeEntity` and deselects if the deleted row was selected (202-210). setContext weakly borrows the scene and clears selection (128-133).

There is no non-ImGui panel API. Every create/delete/select path above runs inside `onImGuiRender` behind ImGui hit testing. The only ways to reach the behavior are (a) real widget interaction with synthesized input events, as Tests/src/EntityTagsCheck.cpp does with a private ImGui context (EntityTagsCheck.cpp:41-248), or (b) bypassing the panel and calling `Scene::createEntity` / `Scene::removeEntity` directly, which tests the engine ops, not the panel. No CLI click-the-panel API exists, and none should be invented for this map.

## Friend fixtures (existing test seams)

| Fixture | Friendship | What it drives |
| --- | --- | --- |
| EditorIssue12Fixture | Editor/src/EditorLayer.h:39 | Full editor loop via `application.run()`: real onAttach/onUpdate/onImGuiRender, framebuffer follows viewport, camera resize delivery, tag edit, component removal, entity deletion (Tests/src/EditorIssue12Fixture.cpp:19-103, 105-112). |
| EditorIssue15Fixture | Editor/src/EditorLayer.h:41 | Real Save As/Open through native dialogs on a private desktop with a UIAutomation driver, plus viewport init through newScene/openScene (Tests/src/EditorIssue15Fixture.cpp:310-509, dialog driver 148-293, viewport run 319-373). |
| ViewportPixelsFixture | Editor/src/EditorLayer.h:42, Engine/Source/Rubber/Scene/Scene.h:72 | onAttach + onUpdate + one onImGuiRender per iteration with pinned ImGui/GL backends; pixel quantization, attachment stability, camera ratio, dockspace style restore (Tests/src/ViewportPixelsCheck.cpp:33-161). |
| EntityTagsFixture | Editor/src/Panels/SceneHierachyPanel.h:32 | Panel-only on a private ImGui context: select, rename (UTF-8, 8192+ bytes), deselect, Delete Entity, Add Component Camera/Visibility, Primary and isVisible checkbox edits (Tests/src/EntityTagsCheck.cpp:41-248). |
| MinimizeRestoreFixture | Engine/Source/Rubber/Core/Application.h:59 | Application frame policy with an EditorLayer subclass (Tests/src/MinimizeRestoreCheck.cpp:23-57). |

Note: `Scene::createEntity`/`removeEntity` (Engine/Source/Rubber/Scene/Scene.cpp:15-29) need no friendship; Regression.cpp:67-92 drives them directly.

## Proposed saveSceneTo(path) / openSceneFrom(path) call-site count

Extracting the dialog-free bodies:

- `openSceneFrom(path)` body = Editor/src/EditorLayer.cpp:210-216. Exactly 1 production call site would remain: `openScene` (206). Menu item (143) and Ctrl+O (240) keep calling the dialog version.
- `saveSceneTo(path)` body = Editor/src/EditorLayer.cpp:224-225. Exactly 1 production call site would remain: `saveScene` (220). Menu item (147) and Ctrl+S (257) keep calling the dialog version.

Grep over the whole tree confirms no other production callers of these private methods. Existing test sites that would keep exercising the dialog versions: 5 saveScene dialog-wrapped calls and 4 openScene dialog-wrapped calls plus 1 direct `SceneSerializer::serialize` (Tests/src/EditorIssue15Fixture.cpp:329, 361, 425, 429, 435, 437, 445, 447, 456, 459). `newScene` is already dialog-free and has 1 fixture call site (Tests/src/EditorIssue15Fixture.cpp:357). Both extracts are straight body moves with a single production caller each, so the migration is two small refactors, not a sweep.

## Surface classification

Flags: GL = needs-GL, ImGui = needs-ImGui, Dialog = needs-native-dialog, Singleton = singleton-locked (Application/WindowInput/FontManager statics), Pure = pure-logic-unit.

| Surface | Evidence | GL | ImGui | Dialog | Singleton | Pure |
| --- | --- | --- | --- | --- | --- | --- |
| New (newScene) | Editor/src/EditorLayer.cpp:198-204 | yes | no calls, but prior onAttach needed it | no | yes (Scene events, window) | no |
| Open (openScene) | Editor/src/EditorLayer.cpp:206-218 | yes (Scene + framebuffer lifetime) | no | yes (only production path) | yes | no |
| Save (saveScene) | Editor/src/EditorLayer.cpp:220-227 | no GL calls; Scene lifetime drags Application/GL in | no | yes (only production path) | yes (Scene) | no |
| Shortcut dispatch | Editor/src/EditorLayer.cpp:229-264 | no | no | no | yes (Input polls glfwGetKey, WindowInput.cpp:10-14) | no |
| Entity lifecycle (create/remove) | Engine/Source/Rubber/Scene/Scene.cpp:15-29 | yes (Scene::create builds GL FBO, RenderSystem.cpp:131-137) | no | no | yes (EventManager hooks, Scene.cpp:159-164) | registry ops pure, Scene construction not |
| Inspector editable fields | Editor/src/Panels/SceneHierachyPanel.cpp:213-369 | no | yes (only path) | no | no | no |
| Hierarchy create/delete/select | Editor/src/Panels/SceneHierachyPanel.cpp:135-211 | no | yes (only path) | no | no | no |
| FontManager | Editor/src/Utilities/FontsManager/FontManager.cpp:19-62 | no (atlas build deferred; FontAtlasCheck runs before glfwInit, Tests/src/Regression.cpp:127-131 vs 132) | yes (context + atlas) | no | process-global `s_FontsMap` (FontManager.cpp:16) | no |
| SceneSerializer | SceneSerializer.cpp:153-176, 183-265 | no direct GL; Scene lifetime requires Application/GL | no | no | yes (Scene) | no |
| Viewport init (initializeSceneViewport) | Editor/src/EditorLayer.cpp:188-196 | yes (m_FrameBuffer) | no | no | yes | no |
| Viewport quantization helper | Editor/src/EditorLayer.cpp:8-14 | no | no | no | no | yes, but anonymous-namespace, reachable only via onUpdate/init |
| TransformComponent data + getTransformMatrix | Engine/Source/Rubber/Scene/Utili/Component.h:17-41 | no | no | no | no | yes |
| Dockspace/menu/stats/viewport presentation | Editor/src/EditorLayer.cpp:75-186 | yes (color attachment for ImGui::Image, 180) | yes | no | yes (Renderer2D stats, Application timer) | no |
| fileDialog open/save | WindowsPlatformUtil.cpp:14-43 | no | no | yes (modal, HWND owner) | yes (Application window, UI thread) | no |

## Can New/Open/Save/createEntity/Transform be driven without onImGuiRender?

- New: yes at the method level. `newScene` makes no ImGui call. It still needs a live Application, a GL context, and a prior `onAttach` (for `m_FrameBuffer`). Reachable today only via the friend fixtures or Ctrl+N, which polls real key state and is not synthetically drivable. The menu item is ImGui-only.
- Open: the body is ImGui-free, but every production user path goes through the native modal dialog. Without a new `openSceneFrom(path)` seam, the only existing driver is the UIAutomation dialog rig in Tests/src/EditorIssue15Fixture.cpp:148-293, which requires an Application window, a private desktop, and the real Win32 dialog.
- Save: same shape as Open. `saveSceneTo(path)` is the seam that would make it dialog-free.
- createEntity: yes. `Scene::createEntity` is a direct engine API (Engine/Source/Rubber/Scene/Scene.cpp:15-24) already driven without any panel (Tests/src/Regression.cpp:67-92). The hierarchy "Create Entity" menu is ImGui-only and has no non-ImGui editor wrapper.
- Transform: the data is plain fields with a pure matrix builder (Component.h:17-41), freely mutable and serializable without ImGui. The user-facing editing path (Inspector drag bars) is ImGui-only. No existing check drives the drag floats; Tests/src/EntityTagsCheck.cpp:168-204 covers the tag input and the two checkboxes only. Driving Inspector editing headlessly requires a real ImGui context with synthesized input events, as EntityTagsCheck demonstrates.

## YAML fields the serializer expects

Written (SceneSerializer.cpp:91-134, 153-176): top map `Scene: Untitled`, `Entities` sequence; per entity `Entity: 123123141` placeholder id, `TagComponent.Tag`, `TransformComponent.Position/Rotation/Scale` (vec3 flow sequences), `CameraComponent` containing `Camera.ProjectionType/PerspectiveFovY/PerspectiveNearClip/PerspectiveFarClip/OrthographicSize/OrthographicNearClip/OrthographicFarClip` plus `Primary` and `FixedAspectRatio`, `SpriteComponent.Color` (vec4), `VisibilityControl.IsVisible`. `saveToFile` creates parent directories and asserts the stream opened (136-146).

Read (183-265): missing top-level `Scene` returns false (190-191); missing `Entities` sequence returns false (196, 263-264); each entity requires `Entity` as uint64 or throws (202); optional `TagComponent.Tag` names the created entity (205-210); optional component blocks add Transform (213-220), Camera (223-239), Sprite (242-249), VisibilityControl (251-258) with typed field reads that throw on shape mismatch. The load is append-only into the context scene and not transactional; failure can leave partial entities (documented at SceneSerializer.h:27-31). Textures, scripts, runtime state, and entity identity do not round trip (SceneSerializer.h:8-15).

## Verdict per required surface

- New/Open/Save: classified above with file:line (Editor/src/EditorLayer.cpp:198-204, 206-218, 220-227; dialog at WindowsPlatformUtil.cpp:14-43; serializer at SceneSerializer.cpp:153-265). Dialog-free extraction needs 1 production call site each.
- Entity lifecycle: create at Engine/Source/Rubber/Scene/Scene.cpp:15-24, remove at 26-29, camera hooks at 107-111/153-156, already driven without ImGui at Tests/src/Regression.cpp:67-92.
- Inspector editable fields: full list with line numbers under the drawComponent trace. ImGui is the only path; EntityTagsCheck is the proven headless strategy.
- FontManager: loadFontFromTTF/getFont/setDefaultFont at Editor/src/Utilities/FontsManager/FontManager.cpp:19-62, fully driven without GL or Application by Tests/src/FontAtlasCheck.cpp:34-112 on a private ImGui context.
- Viewport init: initializeSceneViewport at Editor/src/EditorLayer.cpp:188-196 with the sampling/update split (178-186, 57-73) and pure quantizer at 8-14, proven by Tests/src/ViewportPixelsCheck.cpp and the F06 viewport run in Tests/src/EditorIssue15Fixture.cpp:319-373.

PASS

pwd: C:\Users\yhl20\.t3\worktrees\GameFramework-rb-test-layering
git branch --show-current: test/rb-baseline-layering
git rev-parse HEAD: 494c32c4ec085f83e78b7ba21b258f781a25b567
