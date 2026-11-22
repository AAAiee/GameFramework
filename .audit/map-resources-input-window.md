# Testability map: Resources, Animation, Input, Window, Platform/Windows

Worktree: `GameFramework-rb-test-layering`. Inspect only. All paths relative to repo root.
Core question answered up front: the default runtime does not run the animation
system or the scene input system. `Scene::systemsInit` installs two systems only
(Scene.cpp:53-54, `WindowSystem` then `RendererSystem`), and `Scene.h:20` states
"Other scene systems are dormant." Grep across the whole repo finds no
instantiation of `AnimaitonSystem` or `InputSystem` anywhere; both appear only as
class definitions plus includes from `Rubber.h:55,60`. What does run per frame:
`Application::runFrame` polls the window, dispatches queued events, updates
layers at a fixed 1/120s step, renders ImGui (Application.cpp:139-179), and in
the Editor `EditorLayer::onUpdate` calls `Scene::onSceneUpdate` (EditorLayer.cpp:71),
which updates only the two installed systems (Scene.cpp:70-81).

## Resources (AssetManager, AssetImporter, TextureImporter)

User-facing behavior. `AssetManager::loadAsset(metadata)` imports an asset through
`AssetImporter::importAsset` and registers it under a name-derived handle
(AssetManager.h:18-24). `isAssetLoaded`/`getAsset`/`getAssetMetadata` look it up.
`AssetHandle` is an FNV-1a hash of the name with zero remapped to one, no
collision handling (Asset.h:14-16, Asset.cpp:6-10). Duplicate names assert in
`addAsset` (AssetManager.h:29).

Singleton/static locks.
- `Application::s_Instance` gates the manager, not the manager itself.
  `AssetManager` is a plain instance class, but the engine reaches it via
  `Application::getAssetManager()` (Application.cpp:117-120), which dereferences
  the Application singleton created in the Application ctor
  (Application.cpp:44, 69).
- `s_ImportFuncMap` is a file-scope global keyed by `AssetType`, non-static
  linkage (AssetImporter.cpp:16-20). Texture2D, AnimationAtlas, and
  AnimationSpriteSheet each map to a TextureImporter function.
- `getMissingTexture()` is a function-local static magenta 1x1 texture built on
  first use (TextureImporter.cpp:14-26). Missing atlas slots share this one
  process-lived fallback; its destruction after GL teardown is a documented
  defect (TextureImporter.h:18-20).
- `TextureImporter::s_MissingTexture` is declared (TextureImporter.h:24) but
  never defined or used anywhere. Latent ODR/link hazard if anyone odr-uses it.

Runs in Scene::systemsInit? No. The manager is owned by Application
(Application.cpp:69), not a scene system. Scene.cpp:51-59 touches no resources.
`RendererSystem` reads the manager via the Application static (RenderSystem.cpp:74
via AnimationSystem analog; RendererSystem.cpp:12-17 includes), and the dormant
AnimaitonSystem does the same (AnimationSystem.cpp:74).

GL dependency. `importTexture2D` calls `Texture2D::create(path)`
(TextureImporter.cpp:33), a static factory resolved to GLTexture2D. Texture.h:8-11
requires a live compatible GL context current on the calling thread for creation,
upload, and release. `loadTAnimationAtlas` imports `1.png..frameCount.png` in
order, registering missing slots with the shared fallback (TextureImporter.cpp:53-79).
`loadAnimationSpriteSheet` loads one strip texture and wraps its handle
(TextureImporter.cpp:83-102). Atlas and sheet both borrow the manager raw
(AnimationAtlas.h:29-33, AnimationSpriteSheet.h:12-14); the manager must outlive
them.

Testability. Two tiers.
- Pure registry tier: default-construct an `AssetManager`, call `addAsset` with
  any `Asset` (a test fake suffices), `isAssetLoaded`/`getAsset`/
  `getAssetMetadata`. Headless, no GL, no Application.
- Import tier: `loadAsset` reaches GL through `Texture2D::create`. Needs a live
  GL context, nothing else. `AtlasPlaceholdersCheck` proves this: it builds a
  stack `AssetManager manager;` with no Application (AtlasPlaceholdersCheck.cpp:100)
  and drives the full AssetManager -> AssetImporter -> TextureImporter ->
  AnimationAtlas path against an OpenGL 4.5 context, checking frame slots,
  metadata registration, and pixels via `glGetTextureImage`
  (AtlasPlaceholdersCheck.cpp:82-135).

## Animation (AnimationSpec, AnimationTimer, AnimaitonSystem)

User-facing behavior. `AnimationSpec` is mutable playback state: active clip
handle, frame index/count, loop, flip, anchor mode, completion callback
(AnimationSpec.h:16-57). `AnimationClipConfig` is the pure data passed in
(AnimationSpec.h:61-69). `AnimationTimer` is a manually driven timer that fires
at most one timeout per update and subtracts one interval
(AnimationTimer.h:5-10, AnimationTimer.cpp:28-41). `AnimaitonSystem` (note the
typo in the class name, AnimationSystem.h:7) wires `on_construct<AnimationComponent>`
to arm the timer (AnimationSystem.cpp:21, 55-69), ticks it per update, selects
the frame texture from atlas or sheet, and writes it into the SpriteComponent
(AnimationSystem.cpp:25-53, 72-108).

Singleton/static locks. Two very different tiers inside one module.
- `AnimationTimer`: no locks, no GL, no Application. Fully headless-testable.
- `AnimationSpec` default ctor and plain setters (`setFrameCount`,
  `setRunOnce`, `oppositeDirection`, `setAnchorMode`, `setOnfinished`): pure
  state mutation, headless.
- `AnimationSpec(const AnimationClipConfig&)` and `setSpec(config)`: both call
  `Application::getAssetManager()` and assert the handle is an atlas or sheet
  (AnimationSpec.cpp:19-23, 36-40). Hard lock on the Application singleton plus
  a preloaded clip asset. The header itself says "dormant animation system; not
  installed by Scene" and that configured construction requires Application
  asset metadata (AnimationSpec.h:10-15).
- `AnimaitonSystem::onUpdate` and `updateSpriteTexture` also use
  `Application::getAssetManager()` (AnimationSystem.cpp:74) plus GL-backed
  SubTexture2D creation.

Runs in Scene::systemsInit? No. Scene.cpp:53-54 installs only WindowSystem and
RendererSystem. No code anywhere constructs an AnimaitonSystem. Therefore:
DORMANT. Consequences in the default runtime:
- Adding an `AnimationComponent` (Component.h:104-114) constructs an
  AnimationSpec, which for a configured clip still needs the Application and a
  loaded clip handle. The on_construct hook that would arm its timer only
  exists inside the never-installed AnimaitonSystem, so the timer stays unarmed
  and unticked.
- `RendererSystem` renders `SpriteComponent.texture` as-is (RenderSystem.cpp:74-79).
  Without AnimaitonSystem the sprite keeps whatever texture it was given; frames
  never advance. Animation is dormant end to end, not an enabled feature.

Existing coverage. None for playback. `AtlasPlaceholdersCheck` covers the import
side (atlas slots, fallback sharing). No test constructs an AnimationSpec, ticks
an AnimationTimer, or runs AnimaitonSystem. Tests/unit/UnitTests.cpp has one
case, camera-inverse (UnitTests.cpp:11-13).

Testability. `AnimationTimer` is the cleanest headless seam in the engine: drive
`onUpdate(ts)` with chosen deltas, assert callback counts and pause/resume/
restart semantics. `AnimationSpec` playback state is testable headless when fed
`setFrameCount` instead of the configured ctor. Testing the configured ctor,
setSpec, or AnimaitonSystem as-written requires an Application, a loaded clip,
and a GL context; the seam to introduce would be passing the manager or frame
count in instead of fetching `Application::getAssetManager()`.

## Input (Input facade, WindowInput, InputSystem)

User-facing behavior. Two independent input paths exist.
- Polling facade: `Input::isKeyPressed` and friends forward to a singleton
  instance (Input.h:17-40). The Windows/GLFW implementation `WindowInput` queries
  `glfwGetKey`/`glfwGetMouseButton`/`glfwGetCursorPos` on the Application window
  (WindowInput.cpp:10-27) and toggles cursor mode (WindowInput.cpp:29-39).
- Event path: GLFW callbacks in WindowsWindow enqueue Key/Mouse events into the
  EventManager (Windowswindow.cpp:132-196). `InputSystem` subscribes to those
  events and fills each entity's `InputComponent` bindings and world-space mouse
  position (InputSystem.cpp:19-45, 57-80).

Singleton/static locks.
- `Input::s_Instance` is initialized as `new WindowInput()` during static init
  (WindowInput.cpp:8). Process-lived heap object, never deleted. The header
  carries a TODO to remove the singleton design and select the implementation at
  compile time (Input.h:5-6).
- Every static call then requires `Application::getWindow()` to return a live
  GLFW window (WindowInput.cpp:11 etc.), so the facade is doubly locked: the
  Input singleton plus the Application singleton. Calling it with no Application
  crashes inside Application::getWindow.
- `InputSystem` holds no singleton of its own but reads
  `Application::getEventManager()` in init and shutdown (InputSystem.cpp:18, 49)
  and `Application::getWindow()` for the unproject (InputSystem.cpp:105).

Runs in Scene::systemsInit? No. InputSystem is DORMANT: no instantiation anywhere
in the repo, only the Rubber.h:60 include. Nothing fills an `InputComponent`
(Component.h:123-130) in the default runtime.

Does the default runtime run input at all? Yes, on two of the three paths, none
of which go through Scene systems:
- GLFW events flow every frame: window callbacks enqueue, `runFrame` dispatches
  (Application.cpp:155-158), the Application closes on ESC (Application.cpp:76-82).
- The Editor polls the facade for shortcuts: `shortcutCallBack` checks
  `Input::isKeyPressed(RB_KEY_LEFT_CONTROL/RIGHT_CONTROL)` to gate Ctrl+N/O/S
  (EditorLayer.cpp:234, subscription at 47-49). This is live, user-facing
  behavior today.
- The scene InputSystem path is dormant, as above.

Also dormant: `OrthoCameraController::ProcessInputs` polls the facade
(OrthoCameraController.cpp:49-79) but nothing constructs an
OrthoCameraController; only Rubber.h:39 includes it. CameraController.cpp is
entirely commented out.

Testability. The polling facade is not headless-testable: singleton plus
Application plus GLFW window. The event path is the testable seam;
`EventManager` subscriptions can be driven by enqueueing synthetic
KeyPressedEvent/MouseMoveEvent without a window. Testing InputSystem as-written
needs an Application (event manager, window, primary camera for the unproject
assert, InputSystem.cpp:94). The facade would need instance injection or a
seam at the `WindowInput` virtual boundary to test without a live window.

## Window (Window base, WindowsWindow, WindowSystem)

User-facing behavior. `Window` is an abstract base with a static factory
`Window::create` (Window.h:23-44). The only implementation is `WindowsWindow`.
`onUpdate` polls GLFW and swaps buffers (Windowswindow.cpp:42-48). The factory
is defined in Windowswindow.cpp:24-27, so linking the engine on Windows hard-wires
the GLFW/Win32 implementation; there is no platform switch at runtime or build
time.

Singleton/static locks.
- `WindowsWindow::s_GLFWInitialized` guards one-time `glfwInit`
  (Windowswindow.cpp:16, 84-93).
- The window is not a singleton, but Application is: the Application ctor creates
  the window before anything else (Application.cpp:49) and every static accessor
  goes through `Application::getWindow()`.
- Callback data holds `std::optional<Ref<EventManager>> em` (Windowswindow.h:45)
  and every GLFW callback does `data.em.value()->enqueue(...)` (e.g.
  Windowswindow.cpp:119). The Application ctor creates the window at line 49 but
  creates the EventManager and calls `setEventManager` only at lines 52-53, so a
  window exists briefly where an event would throw `std::bad_optional_access`.
- `shutDown` destroys the GLFW window and GLContext but keeps GLFW alive
  (Windowswindow.cpp:201-206).
- `GLContext::init` runs inside window creation (Windowswindow.cpp:99-100), so a
  live GL context arrives with the window, before Renderer::init
  (Application.cpp:61).

Runs in Scene::systemsInit? The window itself, no; Application owns it. What
Scene installs is `WindowSystem`, whose whole job is subscribing to
`ViewPortResizeEvent` (shared name "Viewport Resize", Scene.h:21-22 warns
simultaneous scenes share delivery) and resizing non-fixed-aspect cameras
(WindowSystem.cpp:12-55). RendererSystem::init also reads the window dimensions
for its framebuffer (RenderSystem.cpp:131-136). So a Scene requires a live
Application window before `Scene::create()` is safe, which matches Scene.h:19:
"Requires a live Application, event manager, logger and current GL context."

Testability. Window creation is live-only: GLFW, Win32, GL loader, visible
desktop. The regression suite already runs it live on an isolated private
desktop (Regression.cpp:30-49, `CreateDesktopW` per process) with an Application
and GL context. The testable seams are the EventManager queue (synthetic events)
and `WindowSystem::onViewPortResize` camera math if fed events directly.

## Platform/Windows: fileDialog (PlatformUtil)

User-facing behavior. `fileDialog::openFile(filter)` and `saveFile(filter)` wrap
Win32 `GetOpenFileNameW`/`GetSaveFileNameW` (WindowsPlatformUtil.cpp:14-43). The
Editor uses them for scene Open/Save As (EditorLayer.cpp:208, 222), reachable
from the File menu or the Ctrl shortcuts that poll the Input facade
(EditorLayer.cpp:229-264).

Requirements to run.
- A live Application: `hwndOwner` comes from
  `glfwGetWin32Window(Application::getWindow().getNativeWindow())`
  (WindowsPlatformUtil.cpp:19, 34). No Application, no dialog.
- The dialog is modal on the calling thread, so it must run on the window's UI
  thread.
- Buffer is 260 wchar including the terminator (259 path code units); extended
  paths unsupported (PlatformUtil.h:6-13).
- Cancel and common-dialog failure are indistinguishable; both return an empty
  string and callers must skip writing (PlatformUtil.h:11-13).
- `OFN_NOCHANGEDIR` preserves the working directory; verified by the fixture
  (EditorIssue15Fixture.cpp:505-506).

Singleton/static locks. None of its own. It is locked to the Application
singleton and to Win32, and the header lives under Scene/Utili/Platform while
the implementation lives under Platform/Windows.

Testability. Testable today, but only as a live modal UI test on Windows.
`EditorIssue15Fixture` drives the real dialogs with UIAutomation from a
background thread against a private desktop: programmatic filename submission,
cancel, decline-overwrite, confirm-overwrite, reject-missing-file, and
over-capacity paths (EditorIssue15Fixture.cpp:425-507, dialog driver and
`RegressionDesktop` isolation in Regression.cpp:30-49). Preconditions the
fixture itself asserts: a live ImGui context, and the Application window on the
current thread (EditorIssue15Fixture.cpp:512-524). A headless seam does not
exist; introducing one would mean injecting the path selection (for example a
dialog interface with a fake in tests) rather than mocking Win32.

## Classification summary

| Module | Classification | Evidence |
| --- | --- | --- |
| AssetManager | Instance class; registry ops headless; owned by Application and reached via `Application::getAssetManager()` static | AssetManager.h:11-44, Application.cpp:69, 117-120 |
| AssetImporter / TextureImporter | GL-bound at import; static import map (file-scope global) and process-lived static fallback texture | AssetImporter.cpp:16-20, TextureImporter.cpp:14-26, 33; tested headless-of-Application in AtlasPlaceholdersCheck.cpp:82-135 |
| AnimationSpec | Split: default ctor and setters pure headless; configured ctor/setSpec locked to Application singleton + preloaded clip; header marks the system dormant | AnimationSpec.h:10-15, AnimationSpec.cpp:19-23, 36-40 |
| AnimationTimer | Pure, manually driven, no locks, fully headless-testable; currently untested | AnimationTimer.h:5-10, AnimationTimer.cpp:28-41, Tests/unit/UnitTests.cpp:11-13 |
| AnimaitonSystem | DORMANT; never instantiated; not in Scene::systemsInit | Scene.cpp:51-59, Scene.h:20, repo-wide grep (only Rubber.h:55 include) |
| Input facade | Singleton `s_Instance` (static-init heap, never freed) + Application/GLFW lock; live in the Editor for Ctrl shortcuts | Input.h:5-6, 52, WindowInput.cpp:8, 11; EditorLayer.cpp:47-49, 234 |
| InputSystem | DORMANT; event-driven, no instantiation; InputComponent never filled in default runtime | Scene.cpp:51-59, repo-wide grep (only Rubber.h:60 include) |
| OrthoCameraController input use | DORMANT; polls facade but no constructor caller | OrthoCameraController.cpp:39-79, only Rubber.h:39 includes it |
| Window / WindowsWindow | Platform hard-wired via factory definition; GLFW + GLContext; event-manager optional set after construction (brief bad_optional_access window) | Window.h:43, Windowswindow.cpp:24-27, 84-100, 119, Windowswindow.h:45, Application.cpp:49-53 |
| WindowSystem | Installed and live; viewport-resize to camera resize only; shared subscription name across scenes | Scene.cpp:53, WindowSystem.cpp:12-55, Scene.h:21-22 |
| fileDialog | Live-only, Win32 modal requiring live Application window on the UI thread; cancel vs failure indistinguishable; tested via UIAutomation on a private desktop | WindowsPlatformUtil.cpp:14-43, PlatformUtil.h:6-13, EditorIssue15Fixture.cpp:425-524, Regression.cpp:30-49 |

Default-runtime verdict. Window creation, event flow, ImGui, the two installed
scene systems, the Input polling shortcuts, and the file dialogs all run for a
user today. The scene animation system and the scene input system do not run;
their components are constructible but never ticked. Both are dormant code
reachable only by manually installing the systems, which no first-party caller
does.

PASS criteria check. fileDialog classified (live-only Win32 modal, Application
lock, existing UIAutomation fixture, WindowsPlatformUtil.cpp + EditorIssue15Fixture).
Input classified (facade singleton vs dormant event-driven InputSystem, both
paths traced, Input.h/WindowInput.cpp/InputSystem.cpp). AssetManager classified
(instance vs Application static accessor, GL-bound import tier, proven
headless-of-Application by AtlasPlaceholdersCheck). AnimationSpec classified
(pure playback state vs Application-locked configured ctor, dormant system,
AnimationSpec.h/.cpp + Scene.cpp).
