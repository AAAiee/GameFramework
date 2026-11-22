# Testability map: Renderer and Platform/OpenGL

Goal: separate pure camera math from GPU batch/FBO/shader work so UnitTests (no GPU, no window) knows which seams it can own. Inspection only; no production file changed. Sources read: every file under `Engine/Source/Rubber/Renderer` and `Engine/Source/Platform/OpenGL` (plus `Platform/Windows` input, `Rubber/Core`, `Rubber/Event`, `Rubber/Resources/Asset.h`, `Engine/Build-Engine.lua`, `Tests/Build-UnitTests.lua`, `Tests/README.md`).

Classification key: `pure-logic-unit` (compiles and runs headless, behavior observable without any graphics dependency), `needs-GL` (correct behavior requires a live compatible context, GL headers or GL link symbols), `singleton-locked` (process-wide static state; tests cannot run instances in isolation). A type can carry more than one flag when its surface splits.

## Static Renderer2D lifecycle and GL 4.5 DSA

- `Renderer2D` is an all-static class (ctor/dtor deleted, `Renderer2D.h:26-27`). All state is process-wide: file-scope `RendererData s_Data` (`Renderer2D.cpp:60`) plus static member `Renderer2D::s_Stats` (`Renderer2D.cpp:62`). One renderer instance per process, no reset short of `shutdown()`.
- Lifecycle is owned by the `Renderer` facade: `Renderer::init()` calls `RendererCommand::init()` (which calls through the raw static `new GLRendererAPI()` allocated at static init, `RendererCommand.cpp:10`) then `Renderer2D::init()`; `Renderer::shutdown()` reverses both (`Renderer.cpp:14-33`). Nothing guards double init or use-after-shutdown; `beginScene` before `init` derefs a null shader library, and a second `RendererCommand::init()` after `shutdown()` derefs the nulled `s_RendererAPI` pointer. Base defects, reported, not fixed here.
- `init()` requires a current GL context: it creates VAO/VBO/IBO through factories that dispatch on `RendererAPI::getAPI()`, loads `assets/shader/CommonShader.glsl` from a working-directory-relative path, creates the renderer-owned white texture in slot zero, pre-fills the 60,000-index static pattern, and sets the 32-entry sampler array. Batches hold 10,000 quads; slot zero is white, leaving 31 user textures.
- `beginScene(vpMatrix)` only uploads a uniform; it does not reset batch state. Draw calls append; capacity flushes happen inside `drawTextureQuad`/`drawColorQuad` before appending. `endScene()` flushes; an empty scene draws nothing.
- GL 4.5 DSA inventory: DSA creation/storage (`glCreateBuffers` + `glNamedBufferData`, `glCreateVertexArrays`, `glCreateTextures` + `glTextureStorage2D`/`glTextureSubImage2D`/`glGenerateTextureMipmap`, `glBindTextureUnit`, `glCreateFramebuffers` + `glNamedFramebufferTexture`/`glNamedFramebufferDrawBuffer`/`glCheckNamedFramebufferStatus`). Legacy global-state calls remain in: attribute setup (`glBindVertexArray` + `glVertexAttribPointer` in `GLVertexArray::addVertexBuffer`), all `bind()`/`unbind()` paths, the whole shader compile/link path (`glCreateShader`/`glCreateProgram`, not DSA), and `GLRendererAPI` (`glViewport`/`glClear`/`glDrawElements`). The existing `gl-boundaries` Regression selector already pins the floor at `GLAD_GL_VERSION_4_5`; shaders are GLSL 450.

## Per-type classification

### Camera — `pure-logic-unit`

Owns one `glm::mat4` projection. User-facing behavior (`Camera.h` contract): `setProjection` copies; `getProjection()` borrows storage (mutable overload allows writes through it); `getInverseProjection() const` returns an owned inverse snapshot that survives later camera changes; callers must supply an invertible matrix (no singular-matrix validation). No view matrix, no GL, no engine headers.

Header-only: not quite. Every method is inline in `Camera.h` except the `Camera(const glm::mat4&)` constructor, whose sole out-of-line definition is `Camera.cpp:6`. The header itself is self-contained (glm includes only), so a test TU consumes the type without `pch.h`. The existing `CameraInverseCheck.cpp` default-constructs and calls `setProjection`, so today's UnitTests never needs `Camera.cpp` (whose `pch.h` include would demand the spdlog include path UnitTests does not have).

### OrthoCamera — `pure-logic-unit`, fully header-only

View + projection pair; `getVpMatrix()` returns `view * projection`; `setViewMatrix` copies. Defined entirely in `OrthoCamera.h`, glm only. Header carries `TODO::DELETE THIS WHOLE FILE` and `Renderer2D::beginScene(const OrthoCamera&)` is marked legacy "to delete" (`Renderer2D.h:35-36`). Headless tests against it are cheap but write behavior for a type scheduled for removal; flag that risk when picking assertions.

### OrthoCameraController — split: `pure-logic-unit` math subset inside a `singleton-locked` shell

User-facing behavior: aspect/zoom/bounds recomputation (`bounds = ±aspect * zoom`, zoom floor 0.25 via scroll clamp, translation speed `zoom * 5`), view-matrix rebuild from position/rotation with ±180 degree wraparound, `updateAspectRatio`, and event subscription under the fixed names `"Camera Resize"` / `"Camera Zoom"`.

- Pure subset: `updateProjectionMatrix`, `setViewMatrix`, `updateAspectRatio`, the zoom/speed arithmetic in `onMouseScrolled`/`updateCurrentSpeed`. All glm, no GL, observable through `getBounds()`/`getCamera()`.
- Singleton-locked subset: `ProcessInputs` dereferences `Input::s_Instance` (static pointer created by `Platform/Windows/WindowInput`; null without a live GLFW window), and subscribe/unsubscribe write `EventBus<EventType>` static per-type storage, so handler registration is process-global.

The TU compiles and links headless (`Input.h`, `KeyCodes.h`, `EventManager.h`, `EventBus.h` are GL-free; `EventManager.cpp` needs only pch/std). Two headless seams exist: drive the pure methods directly, or define `Input::s_Instance` from the test TU against a fake `Input` subclass to make `ProcessInputs` runnable without windowing.

### CameraController — dead code, excluded

Both `CameraController.h` and `CameraController.cpp` are entirely commented out. No user-facing behavior; nothing to test. It is not the type referenced by `OrthoCameraController`.

### Renderer2D — `needs-GL` + `singleton-locked`

User-facing behavior: the documented quad-batch contract in `Renderer2D.h:14-22` and per-method comments (scene bracketing without overlap, 10,000-quad batches, 31 user texture slots with reuse by GL identity, capacity flushes preserving order, UV-pointer borrowing for exactly four coordinates, null selecting full-texture UVs, stats counters under `ENABLE_RENDERER_STATS` which is hardcoded on at `Renderer2D.h:4`).

Texture identity checks compare GL renderer IDs (`GLTexture2D::operator==`), so even the CPU-side batching decisions observe GL-allocated state. All batch behavior therefore belongs behind a real context. It is already covered by the Regression `renderer-batches` selector (F03), which drives the initialized production renderer with a GLAD entry-point observer and framebuffer readback. Per the brief, that oracle is not cloned into UnitTests; the map records it as the owner of this behavior.

Compile note: `Renderer2D.cpp` itself includes no glad (only abstraction headers), so the TU compiles without GL headers. It cannot link without the GL factory TUs and cannot run without a context.

### Renderer (facade) — `needs-GL` + `singleton-locked`

Static facade over `RendererCommand`/`Renderer2D`. `s_SceneDataPtr` is `new`ed at static init (`Renderer.cpp:12`), deleted in `shutdown()`. `beginScene(const Camera&)` and `endScene()` are empty stubs (`Renderer.cpp:39-46`); `submit()` is the legacy bind-and-draw path. `getAPI()` forwards to `RendererAPI::getAPI()`. No headless value; its lifecycle ownership is the thing to respect in fixtures.

### RendererAPI / RendererCommand — `needs-GL` + `singleton-locked`

`RendererAPI::s_API` is statically initialized to OpenGL (`RendererAPI.cpp:7`) with no public setter; the enum has no second implemented member, so API selection is effectively compile-locked. `RendererCommand` owns the raw static `new GLRendererAPI()` (allocated at static init before any context exists; deleted in `shutdown()`, leaving null for a second `init()` to crash on, noted above). `GLRendererAPI` is all legacy global-state GL. Headless tests can only observe the enum/static wiring, which is not behavior worth a test.

### FrameBuffer — three-way split (classified separately from Camera and Renderer2D)

- `FrameBufferSpecification` — `pure-logic-unit`: plain data struct (1280x720 default, `m_Samples` and `m_SwapChainTarget` are stored but unimplemented, per the header contract). Testable as data alone; little value beyond defaults.
- `FrameBuffer` (abstract interface, `FrameBuffer.h`) — header chain is GL-free (`Texture.h` → `Asset.h` → `Utility.h`); compiles anywhere. Behavior exists only through implementations.
- `FrameBuffer::create` factory (`FrameBuffer.cpp`) — `singleton-locked` (switch on `RendererAPI::s_API` under `WINDOW_64_API`) and link-locked to `GLFrameBuffer`. The factory TU itself compiles without glad (`GLFrameBuffer.h` includes no GL header) but links only against glad.
- `GLFrameBuffer` — `needs-GL`, pure DSA. One RGBA8 color + one DEPTH24_STENCIL8 attachment; status asserted complete at creation. `bind()` selects the target and sets the viewport to spec dimensions; `unbind()` selects FBO 0 without restoring prior binding/viewport; `resize()` always recreates attachments (invalidating previously borrowed IDs) even for equal sizes; `recreate()` guards double-creation via `m_RendererID != 0`. This is exactly the kind of context-bound behavior that must stay in the GL regression layer, not UnitTests.

### VertexBuffer / IndexBuffer — interface `pure` to compile, implementations `needs-GL`

Abstract ownership contracts in `Buffers.h` (byte sizes, borrow-only uploads, GL-context lifetime requirement). Factories in `Buffers.cpp` are singleton-locked to `s_API` and link-locked to glad TUs; the factory TU compiles without glad (`GLBuffers.h` is GL-free). `GLVertexBuffer`/`GLIndexBuffer` are `needs-GL` (DSA `glCreateBuffers` + `glNamedBufferData`; count derived from bytes/sizeof(uint32_t)).

### VertexArray — same shape

Abstract interface retains shared ownership of attached buffers (`VertexArray.h` contract). Factory singleton-locked; `GLVertexArray` is `needs-GL`: DSA creation, but attribute setup is the one remaining legacy `glBindVertexArray` + `glVertexAttribPointer` path. Debug-only null guards on accessors.

### BufferLayout / BufferElement — `pure-logic-unit`

No GL anywhere: `getShaderTypeSize` is a byte table over `ShaderType`; the `initializer_list` ctor accumulates offsets and stride; `getComponentCount` maps types to component counts; asserts fire only on unknown types. The TU includes `pch.h`, so compiling it needs the spdlog include path added to the test project, but runtime needs no GL and no Log symbols when asserts are off. Strongest unclaimed headless candidate in this module.

### Shader / ShaderLib — split

- `Shader` abstract interface — GL-free header (`Shaders.h`): bounded-slice paths/uniform names, owned `getName()` borrow.
- `ShaderLib` — container logic (heterogeneous-lookup `unordered_map`, `exists`/`add`/`getShader`, `load` derives the key from the file stem). `add`/`exists`/`getShader` are pure; `load()` calls the `Shader::create` factory, so the class as shipped is `needs-GL` through that one method. The factory TU `Shaders.cpp` includes `GLShader.h`, which includes glad directly (`GLShader.h:8`), so that TU needs glad even to compile, unlike the other factories.
- `GLShader` — `needs-GL`: classic (non-DSA) compile/link with info-log asserts, `#type` preprocessing (pure text parsing but trapped in the GL TU), file read via `ifstream`, uniform-location cache keyed by `std::string`. The `gl-boundaries` selector already exercises the bounded-name uniform behavior against a real context.

### Texture / Texture2D — interface pure to compile, implementation `needs-GL`

Abstract contracts in `Texture.h` (shared ownership, full-level-0 synchronous replace, tight-row unpack preconditions, stb vertical flip on load). Factory singleton-locked; `Texture.cpp` needs glad to compile (via `GLTexture2D.h`). `GLTexture2D` is `needs-GL`: DSA texture storage with mipmaps, `TightUnpackRows` RAII preserving `GL_UNPACK_ALIGNMENT`, `glBindTextureUnit` binding, and `operator==` comparing GL renderer IDs through a `dynamic_cast` (identity, not content).

### SubTexture2D — `pure-logic-unit`

UV math only: `creatFromIndex` normalizes cell bounds against sheet dimensions (with reversed-X support), the ctor fills four UV corners in bottom-left/bottom-right/top-right/top-left order and computes pixel dimensions, `createFromTexture` gives full or mirrored sheets. The TU includes `pch.h` + `Texture.h` (needs the spdlog include path, no glad). It holds only a `Ref<Texture2D>` and never calls GL itself; the header contract correctly notes that releasing the last sheet owner needs a live context, which is transitive through the real `GLTexture2D`. With a test-double `Texture2D` supplying width/height, all of its behavior is headless. Second-strongest unclaimed candidate.

### GraphicsContext / GLContext — `needs-GL`, the boundary itself

`GLContext::init()` is `glfwMakeContextCurrent` + `gladLoadGLLoader` (`GLContext.cpp:15-25`). Untestable headless by definition; every "requires a current context" contract above flows from here.

## Which TUs UnitTests can compile without GL

Current `Build-UnitTests.lua`: explicit two-TU file list, no Engine link, no PCH, includedirs are doctest, `Tests/src`, `Engine/Source`, `RBVendor/glm`. Verified against each TU's include chain:

Compile clean today (glm-only headers):
- `unit/UnitTests.cpp`, `src/CameraInverseCheck.cpp` (shipped, passing).
- Any new TU including only `Camera.h`, `OrthoCamera.h`, `SubTexture2D.h`'s declarations, or `EventManager.h`/`EventBus.h` types.

Compile clean once `RBVendor/spdlogs/include` is added (sole blocker is `pch.h` → `Log.h` → spdlog; still no GL, no GLFW, no glad):
- `BufferLayout.cpp` — links against nothing but its own logic (asserts compile out without `RB_ENABLE_ASSERTS`).
- `SubTexture2D.cpp` — links with a test-double `Texture2D` defined in the test TU.
- `Camera.cpp` — only if a test truly needs the `mat4` constructor; the inline path avoids it.
- `OrthoCameraController.cpp` — links with `EventManager.cpp` (GL-free); for `ProcessInputs` a fake `Input` subclass plus a test-local definition of `Input::s_Instance` avoids pulling `WindowInput.cpp`/GLFW.

Compile clean but link-locked to GL (do not add; they pull glad symbols):
- `FrameBuffer.cpp`, `Buffers.cpp`, `VertexArray.cpp`, `Renderer2D.cpp` — no glad in their include chains, but every symbol they instantiate lives in `Platform/OpenGL/*.cpp`.

Cannot even compile without glad on the include path:
- `Shaders.cpp`, `Texture.cpp` (glad via `GLShader.h` / `GLTexture2D.h`), and every file under `Platform/OpenGL/`.

## Unclaimed headless surface, ranked

1. `BufferLayout` offsets/stride/component counts — pure, zero seams needed.
2. `SubTexture2D` UV/dimension math — pure, needs one trivial `Texture2D` double.
3. `OrthoCameraController` bounds/zoom/aspect/view math — pure subset of a singleton shell; avoid or fake `Input`.
4. `OrthoCamera` VP composition — trivial but the type is slated for deletion.
5. `Camera` — already covered by `camera-inverse`; only the `mat4` constructor remains untested.

Everything else in this module stays behind a real OpenGL 4.5 context in Regression: `Renderer2D` batch behavior (`renderer-batches`), FBO attachment lifecycle, shader compile/uniform boundaries (`gl-boundaries`), buffer upload/DSA (`gl-buffers`), texture unpack orientation (also `gl-boundaries`). Those selectors own that behavior; this map does not propose duplicating them.

PASS: Camera (`pure-logic-unit`), Renderer2D (`needs-GL` + `singleton-locked`), and FrameBuffer (spec pure / factory singleton-locked / GL impl `needs-GL`) are classified separately.

Worktree: branch `test/rb-baseline-layering`, head `494c32c4ec085f83e78b7ba21b258f781a25b567`.
