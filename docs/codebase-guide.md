# Rubber Engine and Editor: reading guide

Reviewed against accepted base `e3bf3fff0f651b072b1357f234b853b3e2a3eea6` and the issue #25 contract publication. This incorporates the 2026-10-14 local guide and audit drafts, with historical/proposed statements rechecked against source. [The audit](codebase-improvement-plan.md) retains excluded defects; [combined evidence](validation/spec-11.md) separates supplied Windows observations from unrun human checks. No glossary term or new architecture decision is introduced.

## Start here

Read these paths in order to follow one frame from application startup to editor output:

1. [Editor.cpp](../Editor/src/Editor.cpp): `CreateApp()` constructs the editor application and pushes an `EditorLayer`.
2. [EntryPoint.h](../Engine/Source/Rubber/Core/EntryPoint.h): the entry point starts logging, creates the application, runs it, and destroys it.
3. [Application.cpp](../Engine/Source/Rubber/Core/Application.cpp): window, event manager, renderer, timer, and asset manager setup; initial layer attachment; the application loop.
4. [EditorLayer.cpp](../Editor/src/EditorLayer.cpp): editor fonts, active scene, framebuffer, viewport, and file operations.
5. [Scene.cpp](../Engine/Source/Rubber/Scene/Scene.cpp): entity creation, component hooks, and scene system installation/update.
6. [RenderSystem.cpp](../Engine/Source/Rubber/Scene/System/RenderSystem.cpp): camera selection, sprite rendering, and collision debug rendering.
7. [Renderer2D.cpp](../Engine/Source/Rubber/Renderer/Renderer2D.cpp): CPU vertex batching, texture slots, and indexed drawing.
8. [SceneHierachyPanel.cpp](../Editor/src/Panels/SceneHierachyPanel.cpp): entity selection and component editing.
9. [SceneSerializer.cpp](../Engine/Source/Rubber/Scene/Utili/Serializer/SceneSerializer.cpp): the supported `.rubber` file format.

## Ownership and lifetime

| Object | Current owner or storage | What a reader must know |
| --- | --- | --- |
| `Application` | Created by the entry point; also exposed through `s_Instance` | There is currently no separate engine-runtime owner. |
| Window and timer | `Scope<T>` members of `Application` | `Scope` aliases `std::unique_ptr`. |
| Asset manager | `Ref<AssetManager>` in `Application` | `Ref` aliases `std::shared_ptr`; animation resources borrow the manager through a raw pointer. |
| Layers | Raw pointers remaining in `LayerStack` | The stack deletes retained layers. Popping erases a pointer without deleting or retaining ownership of it. |
| Active scene | `Ref<Scene>` in `EditorLayer` | The hierarchy panel holds a `weakRef<Scene>`, which does not extend the scene lifetime. |
| Entity wrapper | Entity ID plus borrowed `Scene*` | It must not outlive its scene. IDs from different scene registries are not interchangeable. |
| Scene systems | `Scope<SystemBase>` objects in `Scene` | Each borrows its scene and registry after initialization. |
| Editor framebuffer | `Ref<FrameBuffer>` in `EditorLayer` | Scene rendering writes into the framebuffer bound by the editor. |
| Event handlers | Static storage in each `EventBus<EventType>` | Captured pointers require explicit unsubscription. Storage can survive an application. |
| Event manager | Shared singleton retained by `EventManager::create()` | Queue storage can survive an application; it is not presently owned exclusively by an engine runtime. |
| Fallback texture | Function-static cache in `TextureImporter.cpp` | Its current lifetime is longer than an individual graphics context. |

The current viewport subscription and teardown code do not establish independent scene/runtime lifetime boundaries. See the audit before extending this area.

## Application loop and events

Initial attachment iterates layers in reverse order. This creates the ImGui context before the editor layer loads its fonts. Updates and ImGui rendering iterate ordinary layers followed by overlays in forward order. Pushing a layer after initial attachment currently does not automatically call its `onAttach()`.

Each loop iteration starts the timer, updates the window, dispatches queued events, performs accumulated fixed updates, renders ImGui, then waits for the frame target. Fixed updates use `1/120s`; the timer is initialized with a 60 FPS target. The timer's delta is from the previous completed frame, not the current frame still being measured. Scene rendering currently runs inside the fixed updates. Minimized iterations keep polling and rendering ImGui but discard lag; restoration skips the preceding minimized delta before resuming fixed updates.

[EventManager.cpp](../Engine/Source/Rubber/Event/EventManager.cpp) dispatches the current queue first, then swaps in the next queue and clears the completed queue. `enqueue()` owns a copy of the event payload and appends to the next queue. A newly enqueued event is promoted by one dispatch call and delivered by the following call; the first call after startup only promotes events. Events enqueued during current-queue dispatch can be delivered on the following call. In [EventBus.h](../Engine/Source/Rubber/Event/EventBus.h), handler names are unique per event type, handlers execute in registration order, and the first handler returning `true` consumes the event.

Registering or unregistering handlers during handler iteration is currently unsafe because it can invalidate the underlying vector. Deferred mutation has not been implemented. Layer deletion currently does not invoke `onDetach()`, so destruction alone does not clean up event subscriptions or ImGui backends.

## Scene data and systems

`Scene::createEntity()` creates an EnTT entity and a `TagComponent`. It does not automatically create a transform or visibility component. The `Entity` templates access the owning scene's registry and invoke scene component hooks. There are explicit hooks for Tag, Transform, Sprite, Camera, and Visibility; other component types do not have the same complete hook support.

`Scene::systemsInit()` installs exactly these two scene systems, in this order:

1. `WindowSystem`: listens for viewport resize events and updates cameras without a fixed aspect ratio.
2. `RendererSystem`: draws the scene into the currently bound framebuffer.

Input, animation, movement, collision, scripts, and FSM code exists in the scene subtree but is not installed by the current default scene setup. Do not assume those systems run in the editor merely because their headers are exposed by `Rubber.h`.

The viewport dimensions are stored in the scene. Adding or removing a camera queues a viewport resize. The current resize event has no scene identity, and all `WindowSystem` instances use the same subscriber name. Supporting simultaneous scenes with independent viewports requires correcting this contract.

## Rendering contracts

The editor binds its off-screen framebuffer around `Scene::onSceneUpdate()`, then displays its color attachment using ImGui. Binding changes the GL viewport; unbinding selects framebuffer zero without restoring the previous viewport. Resizing replaces attachments, so attachment IDs are not stable across resize.

The scene renderer uses `projection * view`, with the view obtained by inverting the camera transform. It uses the last primary camera found in the registry iteration; this is not a stable priority policy when multiple cameras are marked primary. If no usable camera exists, it clears the framebuffer and returns.

Sprites require Transform and Visibility components to enter the drawing view. The current sprite sorting code assumes every Sprite also has a Transform, although the editor allows their independent removal. The audit identifies this mismatch.

`Renderer2D` shares one batch across calls. `beginScene()` supplies the view-projection matrix; `endScene()` flushes. A batch allocates space for 10,000 quads and 32 texture slots. Slot zero holds a white texture, leaving 31 slots for other textures. Both low-level append paths flush before quad capacity is exceeded. Texture draws search for an existing slot before flushing for a new texture; empty flushes do no upload/draw and add no draw-call count.

Buffer allocation/upload sizes and layout offsets/stride are measured in bytes. Indexed draw counts are measured in index elements. Quad UV pointers refer to four normalized texture coordinates; image loading flips images vertically. Texture upload callers provide tightly packed pixel bytes. GPU object operations and destruction require a compatible current GL context.

## Scene files and resources

Scene serialization covers Tag, Transform, Camera parameters/flags, Sprite color, and Visibility. It does not save texture asset references, scripts, animation state, input bindings, collision data, or runtime system state. The serialized entity number is currently a repeated placeholder, not a persistent unique identity. Runtime serialization functions are assertion stubs. A missing or empty tag loads through createEntity and becomes "Unknown Entity"; exact empty-tag round trips are not guaranteed.

Opening a file currently discards the active scene before deserialization, ignores the boolean result, and does not contain YAML parser/conversion exceptions. These are known defects, not a reliable recovery contract. Save As permits new paths and prompts before overwrite. Both dialogs advertise wide-character element capacity and return an empty path on cancel. This does not repair serializer write-error reporting or migrate long-path APIs.

[Asset.h](../Engine/Source/Rubber/Resources/Asset.h) uses deterministic hashes of asset names as handles. These handles are not scene entity IDs or UUIDs. An animation atlas path points to a directory containing numbered images from `1.png` to `frameCount.png`; a sprite-sheet path identifies one horizontal strip. Animation resources borrow their asset manager, which must outlive their lookups.

## Build and validation

The current build is Premake-generated Visual Studio projects, C++20, Windows x64. `Build.lua` includes the engine/editor project files and third-party project definitions. From the repository root, generate without the interactive setup script:

```powershell
& '.\Vendor\Binaries\Premake\Windows\premake5.exe' --file=Build.lua vs2022
```

Use the exact Debug/Release generation, Rebuild, full-suite, selector and editor-script commands in [Tests/README.md](../Tests/README.md#combined-validation-for-issue-25). [The evidence ledger](validation/spec-11.md) records supplied #25 passes at `2f06a8e8bcd59e45e7a459eded58993b547bae5a` and their limits. The original baseline failure and local F01 experiment are historical; the first-party Regression target now exists. Those host runs passed generation, both builds, all selectors and five standalone scripts. The supplied runs include the repair’s Inspector Add Component menu and Primary/Visibility checkbox checks in both builds. Human interactive ordinary editing remains unrun. These observations apply to that exact checkpoint SHA. The ledger also identifies the separate final acceptance report, whose commit field and issue #25 publication establish the final reviewed SHA.

Run the editor from its generated output directory so relative `assets/` paths resolve to the copied fonts and shader. Generated projects and `Binaries/` are ignored build artifacts. Third-party source is under `Engine/RBVendor/` and `Vendor/`.

## Contract map for follow-up reading

These declaration comments are caller contracts; the guide describes their connections. All paths below were inspected, including the corresponding implementations.

| Seam | Declaration contracts | Boundary to retain |
| --- | --- | --- |
| Application / LayerStack | [Application.h](../Engine/Source/Rubber/Core/Application.h), [LayerStack.h](../Engine/Source/Rubber/Layer/LayerStack.h) | Reverse initial attach, forward updates; pop transfers lifetime responsibility without detach/delete. |
| EventBus / EventManager | [EventBus.h](../Engine/Source/Rubber/Event/EventBus.h), [EventManager.h](../Engine/Source/Rubber/Event/EventManager.h) | Exact owned names; no subscription mutation or recursive queue dispatch in callbacks; queued payload copied, immediate payload borrowed. |
| Scene / Entity / SystemBase | [Scene.h](../Engine/Source/Rubber/Scene/Scene.h), [Entity.h](../Engine/Source/Rubber/Scene/Utili/Entity.h), [System.h](../Engine/Source/Rubber/Scene/System/System.h) | Borrowed wrappers/component references can expire on removal, relocation or scene death; truthiness does not prove validity. |
| Rendering | [Renderer2D.h](../Engine/Source/Rubber/Renderer/Renderer2D.h), [Camera.h](../Engine/Source/Rubber/Renderer/Camera.h), [FrameBuffer.h](../Engine/Source/Rubber/Renderer/FrameBuffer.h), [Buffers.h](../Engine/Source/Rubber/Renderer/Buffers.h) | Nonoverlapping batches; owned inverse snapshot; bytes versus index elements; attachment IDs expire on resize; live GL context through final release. |
| Texture / Subtexture | [Texture.h](../Engine/Source/Rubber/Renderer/Texture.h), [SubTexture2D.h](../Engine/Source/Rubber/Renderer/Subtexture2D.h) | Shared sheet ownership, four borrowed normalized UVs; cell counts versus pixel dimensions; packed RGB/RGBA and unpack-state requirements. |
| Serialization | [SceneSerializer.h](../Engine/Source/Rubber/Scene/Utili/Serializer/SceneSerializer.h) | Shared scene, supported fields only, partial mutation and throwing YAML conversion. |
| Editor / hierarchy | [EditorLayer.h](../Editor/src/EditorLayer.h), [SceneHierachyPanel.h](../Editor/src/Panels/SceneHierachyPanel.h) | Previous panel size feeds fixed update; weak hierarchy context; literal dynamic UTF-8 tags, stable entity row IDs, font glyph limits. |
| Fonts | [FontManager.h](../Editor/src/Utilities/FontsManager/FontManager.h) | Positive pixel size; trusted TTF/OTF loaded before frames; pointers borrow the current atlas, lookup can return null, failed load throws with path. |
| Animation resources | [AnimationAtlas.h](../Engine/Source/Rubber/Resources/AnimationResources/AnimationAtlas.h), [AnimationSpriteSheet.h](../Engine/Source/Rubber/Resources/AnimationResources/AnimationSpriteSheet.h), [AnimationSpec.h](../Engine/Source/Rubber/Animation/AnimationSpec.h), [AnimationTimer.h](../Engine/Source/Rubber/Animation/AnimationTimer.h) | Manager borrowed for lookups; atlas positions preserved with placeholders; sheet is one horizontal strip, zero-based columns; playback systems dormant. |

SceneCamera perspective FOV and Transform rotation use radians, while orthographic size is vertical scene extent; viewport dimensions are pixels. Near/far values use scene units. Do not infer validated camera ranges from setters. The camera inverse requires an invertible projection and returns by value.

Font names use exact bounded lookup and retain atlas pointers rather than insertion indices. Unknown lookup returns null without insertion; unknown default selection returns false. The static name registry must stay within a single atlas lifetime. AnimationSpec copies settings and gets frame count from Application metadata; it does not install an animation system. Timer callbacks run inline, at most once per update. Existing nonloop completion, callback-reference and timer catch-up defects remain in the audit.
