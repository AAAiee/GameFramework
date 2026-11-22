# Testability map: Rubber Core / Event / Layer (no-GPU UnitTests baseline)

Inspect-only audit of `Engine/Source/Rubber/{Core,Event,Layer}` at HEAD
`494c32c4ec085f83e78b7ba21b258f781a25b567` on branch `test/rb-baseline-layering`.
No production file changed. Flags come from call and link edges in the sources
(includes, constructor bodies, static state), not from issue trackers. No MSVC
toolchain exists on this host, so nothing here was compile-verified; every claim
below is from reading code and is labeled by file and line.

Flag legend: `pure-logic-unit` (compilable and runnable with no GPU, no window,
no ImGui), `needs-GL`, `needs-ImGui`, `needs-native-dialog`, `singleton-locked`
(construction restricted to one process instance, or reachable only through a
process-lifetime singleton accessor).

## Global compile context (applies to every module below)

- **pch.h chain.** Every `.cpp` in the three trees opens with `#include <pch.h>`
  (`Engine/Source/pch.h:1`). Unusually, `Event/Event.h:1` also includes
  `"pch.h"` directly, so even header-only use of any Event type transitively
  pulls the whole pch: STL headers, then `Rubber/Core/Log.h` (spdlog headers,
  `Log.h:3-4`), `Rubber/Debug/Profiler.h`, `Rubber/Core/Assert.h`,
  `Rubber/Core/Utility.h`. Compiling any Event code therefore needs the
  `Engine/RBVendor/spdlogs/include` directory (`Build-Engine.lua:18,29` names
  it) even though spdlog is used header-only (no spdlog entry in Engine `links`,
  `Build-Engine.lua:39-46`).
- **WINDOW_64_API.** `pch.h:29-32` includes `<Windows.h>` and
  `#pragma comment(lib, "winmm.lib")` only under `WINDOW_64_API`, which the
  workspace defines globally (`Build.lua:10`). UnitTests must reproduce that
  define (or lose `Windows.h`, `timeBeginPeriod`, and the winmm pragma).
- **Assert/log macros.** `RB_CORE_ASSERT` expands to `RB_ERROR(...)` plus
  `RB_DEBUGBREAK()` under `RB_ENABLE_ASSERTS` (`Assert.h:5-22`); `RB_ERROR`
  dereferences `Logger::GetCoreLogger()` (`Log.h:35`). With `RB_ENABLE_ASSERTS`
  unset the assert macros are empty (`Assert.h:27-31`). Debug builds define both
  `RB_DEBUG` and `RB_ENABLE_ASSERTS` (`Build-Engine.lua:61`), and `Core.h:12-14`
  also auto-defines `RB_ENABLE_ASSERTS` from `RB_DEBUG`.
- **Logger::Init requirement.** `Logger::Init` is what gives the two loggers
  their `shared_ptr` values (`Log.cpp:30-37`); `Logger::Get` is a Meyers
  singleton (`Log.cpp:50-54`). In Debug, any firing assert or any `RB_*`/`APP_*`
  log call dereferences a null `shared_ptr` until `Init` has run. Production
  callers get this from `EntryPoint.h:14-16`, which calls
  `Rubber::Logger::Init()` first thing in `main`. A UnitTests binary has its own
  `main` (doctest), so it must call `Logger::Init()` itself before exercising
  assert-guarded paths (notably `EventBus::registerEventHandler`/
  `unregisterEventHandler`, which assert on duplicate and missing names,
  `EventBus.h:31,37,49`). A second `Init` call recreates the same named spdlog
  loggers; spdlog's registry rejects duplicate logger names (inferred, not
  measured here), so treat `Init` as once-per-process.
- **EntryPoint.h defines `main()`** under `WINDOW_64_API` (`EntryPoint.h:2,12`).
  UnitTests must never include it; it would collide with doctest's main and
  hardwire `CreateApp()`/`run()`.
- **Shared include-path minimum for a no-Engine-link UnitTests** (superset of
  the current `Tests/Build-UnitTests.lua:13-18`, which already has
  `Engine/Source` and glm): add `Engine/RBVendor/spdlogs/include` (pch chain)
  and `Engine/RBVendor/glfw/include` with `GLFW_INCLUDE_NONE` (only because
  `GameLoopTimer.cpp:2` includes `<GLFW/glfw3.h>`; it calls no GLFW function,
  and `GLFW_INCLUDE_NONE` keeps GL headers out).

## Module: Core (`Engine/Source/Rubber/Core`)

Files: `Application.{h,cpp}`, `GameLoopTimer.{h,cpp}`, `Log.{h,cpp}`,
`Utility.{h,cpp}`, `Core.h`, `Assert.h`, `Search.h`, `EntryPoint.h`.

### Application — flags: `needs-GL` | `needs-ImGui` | `singleton-locked`

Not a pure-logic-unit; every path below is from `Application.cpp`.

Existing user-observable behavior:

- Construction: creates a window titled with the given name, creates the
  shared event manager, pushes the ImGui overlay (updated last, rendered on
  top), initializes renderer and RNG, disables vsync, paces at 60 FPS, and
  subscribes window close, window resize, and ESC-to-close
  (`Application.cpp:39-84`).
- `run()` attaches all layers once in reverse stack order (overlays first,
  `Application.cpp:126-129,237-243`), then loops: start timer, poll window,
  flush the event queue, fixed-update layers, ImGui render pass, pace
  (`Application.cpp:139-179`).
- Fixed updates run at 1/120 s (`FIXED_TIME_STAMP`, `Application.cpp:37`); time
  is accumulated and leftovers carry (`updateFixedLayers`,
  `Application.cpp:181-194`). A zero-extent resize marks the app minimized
  (`Application.cpp:215-219`); while minimized, or on the first frame after
  restore, updates and accumulated lag are discarded, but ImGui still renders
  (`Application.cpp:183-186,165-173`).
- Window close, ESC key, and `Application::close()` all stop the loop
  (`Application.cpp:197-207,76-82`).
- Static accessors `getWindow/getEventManager/getTimer/getAssetManager` borrow
  the single instance's facilities (`Application.h:35-43`); `pushLayer` inserts
  before overlays, `pushOverlay` appends (`Application.cpp:102-110`).

Call and link edges forcing the flags:

- `needs-GL`: the constructor calls `Window::create` → `WindowsWindow::init` →
  `glfwInit`/`glfwCreateWindow` + `GLContext::init` (GL loader;
  `Windowswindow.cpp:84-100`, includes `<glad/glad.h>` at line 2), and
  `Renderer::init()` → `RendererCommand::init`/`Renderer2D::init`
  (`Renderer.cpp:14-18`) → `GLRendererAPI::init` → `glEnable`/`glBlendFunc`
  (`GLRendererAPI.cpp:9-10`). `onWindowResize` calls `Renderer::onWindowResize`
  → `glViewport` (`Application.cpp:225`; `Renderer.cpp:20-23`;
  `GLRendererAPI.cpp:20`). The destructor calls `Renderer::shutdown`
  (`Application.cpp:86-90`).
- `needs-ImGui`: the constructor pushes a real `ImGuiLayer`
  (`Application.cpp:56-57`), and every frame calls `m_ImGuiLayer->begin()/end()`
  around per-layer `onImGuiRender` (`Application.cpp:167-172`). `ImGuiLayer`
  itself needs GLFW and GL backends (`ImGuiLayer.cpp:57-58,69-70,81`) and
  `Application::getWindow()` (`ImGuiLayer.cpp:53`), so it cannot be stubbed out
  without changing production code.
- `singleton-locked`: `s_Instance` is a raw static pointer set in the
  constructor and asserted null-first (`Application.h:66`, `Application.cpp:36,43-44`).
  The destructor never clears it (`Application.cpp:86-90`), so a second
  `Application` asserts even after the first is destroyed, and the static
  accessors dereference `s_Instance` without a null check
  (`Application.cpp:92-120,197-200`). Restarting a closed `Application` is
  outside the API contract (`Application.h:18-26`); `run()` after `close()`
  returns immediately since `m_Runing` stays false (`Application.cpp:132`).
- Also compiles against `Platform/Windows/Windowswindow.h`, `Rubber/Input/*`,
  `Rubber/Renderer/*`, `Rubber/Resources/AssetManager.h`
  (`Application.cpp:14-31`), so even compiling this TU (not just linking) drags
  in the platform and renderer trees.

TU verdict: exclude `Application.cpp` from a no-GPU UnitTests. Its frame policy
(`updateFixedLayers`, `onWindowResize` minimize logic) is testable only through
the `MinimizeRestoreFixture` friend (`Application.h:59`), which drives the real
window and stays in the GPU-bound Regression suite
(`Tests/src/MinimizeRestoreCheck.cpp:23,59-114`).

What constructing `Application` requires today: `Logger::Init` already run
(documented at `Application.h:30`; enforced de facto by assert/log macro
shapes), no prior instance alive, a GLFW window with a GL context, the
  process-singleton `EventManager::create()`, an initializable GL renderer,
  `RandomEngine::init`, and winmm-backed `GameLoopTimer(60)`.

### GameLoopTimer — flags: `pure-logic-unit`

CPU-only; no GL, ImGui, or window edge. Windows-only through winmm, which is a
link need, not a GPU need: `timeBeginPeriod(1)` in the constructor and
`timeEndPeriod(1)` in the destructor (`GameLoopTimer.cpp:17,23`), resolved via
`Windows.h` and linked by the pch's `#pragma comment(lib, "winmm.lib")`
(`pch.h:29-32`). The `<GLFW/glfw3.h>` include (`GameLoopTimer.cpp:2`) is
vestigial; no GLFW symbol is called (whole file read, lines 1-99). One
fragility for a standalone TU: it uses `std::this_thread::sleep_until`/
`yield` (`GameLoopTimer.cpp:61,66`) with no direct `<thread>` include; inside
Engine it resolves through the pch chain. UnitTests should confirm or add the
include when it lifts this TU.

Existing user-observable behavior: measured FPS updates about once a second
with rounding (`GameLoopTimer.cpp:42-48`); frame pacing sleeps most of the
remaining budget then busy-waits, tracking sleep overshoot as a compensating
error (`GameLoopTimer.cpp:52-75`); `getDeltaTime` returns the seconds of the
last completed frame, or the target duration before any completed frame
(`GameLoopTimer.h:16-19`, `GameLoopTimer.cpp:77-79,93-97`); `getAverageFrameTime`
averages up to 60 retained samples (`GameLoopTimer.cpp:87-91`);
`getElapsedTime` is seconds since `startFrame` (`GameLoopTimer.cpp:82-85`).

What constructing it requires today: nothing but winmm
(`timeBeginPeriod`); freely constructible, any count, no singleton.

### Logger — flags: `pure-logic-unit`

Header-only dependency on vendored spdlog; `Log.cpp` needs the spdlog include
dir and glm (its fmt specializations for `glm::vec3`/`glm::vec4`,
`Log.cpp:2-25`). `Init` builds "Rubber" and "Client" stdout color loggers at
trace level with a `%^[%T] %n: %v%$` pattern (`Log.cpp:30-37`). Copy assignment
and construction are deleted (`Log.h:17-18`); the instance is a Meyers
singleton (`Log.cpp:50-54`), which is construction detail, not a test lock:
tests never construct `Logger`, they call `Init` once.

What constructing it requires today: nothing user-visible; `Init()` is the
entry point and is a prerequisite for every log/assert macro in Debug.

### RandomEngine (`RandomEngineImpl`, Utility.h) — flags: `pure-logic-unit`

`init` seeds a static `std::mt19937` from `std::random_device`
(`Utility.h:58-60`); `Float` returns a uniform [0,1] float
(`Utility.h:62-65`). The static members are defined in `Utility.cpp:8-9`, so
any TU using `RandomEngine::Float` must also compile `Utility.cpp`.

### HashFunc::fnv1a64 (`Utility.h:79-88`) — flags: `pure-logic-unit`

`constexpr` FNV-1a over `string_view`; header-only, no state.

### `stringHash` (`Search.h`) — flags: `pure-logic-unit`

Transparent hasher for `const char*`/`string_view`/`std::string` enabling
heterogeneous unordered lookup; header-only, no state. Used by `EventBus`'s
name pool.

### Container/smart-pointer aliases (`Utility.h`: `Vector`, `Deque`, `Ref`,
`Scope`, `weakRef`, `makeRef`, `makeScope`) — flags: `pure-logic-unit`

Thin aliases over STL (`Utility.h:8-49`). Every type in the three trees that
stores state uses these, so their behavior (std containers) is the baseline.

### Include surface that is not a type: `Core.h` (`BIT`, `RB_DEBUGBREAK`,
debug-assert auto-enable), `Assert.h` (assert macros), `EntryPoint.h` (`main`).
Classified above where they matter; nothing in them links anything.

## Module: Event (`Engine/Source/Rubber/Event`)

Files: `Event.h`, `EventBus.h`, `EventManager.{h,cpp}`, `AppEvent.h`,
`KeyEvent.h`, `MouseEvent.h`. All types are header-only except `EventManager`'s
two out-of-line members.

### `EventType`, `EventCategory` (enums), `toUnderType` — flags: `pure-logic-unit`

Closed enum identities for every event kind (`Event.h:24-32`) and bitmask
categories (`Event.h:40-48`). Bit-test via `isInCategory`
(`Event.h:104-107`).

### `Event` (abstract base) — flags: `pure-logic-unit`

Interface: `getEventCategory`, `getEventType`, `getTypeName`, `toString`
(defaults to the type name, `Event.h:97`), streamable via `operator<<`
(`Event.h:124-127`), and a mutable handled flag readable through `isHandled`
and writable through `setHandleStatus` even on const events (`Event.h:109-119`).

### `EventBus<T>` — flags: `pure-logic-unit`

Existing user-observable behavior: per-event-type named subscriptions in a
static registry; names are owned copies, must be nonempty and unique per type
(asserts at `EventBus.h:30-32,37`); registration order is delivery order; the
first handler returning true consumes and stops delivery and marks the event
handled (`EventBus.h:59-69`); already-handled events are skipped
(`EventBus.h:61`); unregister removes by exact name and asserts if absent
(`EventBus.h:43-57`); registering or unregistering during delivery is
documented unsupported (`EventBus.h:18-20`).

Caveat for tests: all state is per-type process-global statics defined in the
header (`EventBus.h:101-109`), so every test in a process shares one registry
per event type. Isolation means subscribing under unique names and
unsubscribing in teardown (the pattern `Tests/src/EventChecks.cpp:15-27`
already uses). No TU needed; no lock on construction (there are no instances).

### `EventManager` — flags: `pure-logic-unit` | `singleton-locked`

Existing user-observable behavior: `create()` hands out one process-lifetime
manager (static `Ref` cached forever, `EventManager.cpp:18-25`); `enqueue`
stores a copy of the event in a next queue (payload copies keep their enqueue-
time handled status; caller-owned references stay the caller's lifetime
responsibility, `EventManager.h:31-34`, `EventManager.cpp:78-86`);
`dispatchAllEvents` delivers the current queue in insertion order, then
promotes the next queue, so newly enqueued events need two calls and events
enqueued during delivery arrive on the following call (`EventManager.cpp:8-16`,
documented `EventManager.h:8-13`); `dispatchEvent` invokes one callback
synchronously on a type match, overwrites the handled status with the
callback's result even if the event was already handled, and reports only the
type match (`EventManager.h:38-64`). No synchronization anywhere
(`EventManager.h:13`).

Singleton-lock evidence: the constructor is private (`EventManager.h:51`) and
`create()` caches a never-reset static `Ref` (`EventManager.cpp:20-24`), so a
test process gets exactly one manager; queue state and the per-type EventBus
registries persist across `Application` destruction (documented
`EventManager.h:7-8`). Tests must share it and clean their subscriptions.

TU verdict: UnitTests must compile `EventManager.cpp` (the only non-inline
members are `dispatchAllEvents` and `create`). No Engine link needed beyond it.

### App events — `WindowResizeEvent`, `WindowCloseEvent`, `ViewPortResizeEvent`,
`AppTickEvent`, `AppUpdateEvent`, `AppRenderEvent` — flags: `pure-logic-unit`

Payload constructors and accessors with `toString` strings
(`AppEvent.h:12-123`). All carry `EventCategory::APPLICATION`. Observable
quirk worth knowing before asserting on strings: `ViewPortResizeEvent::toString`
concatenates width and height with no separator (`AppEvent.h:58-61`). The
zero-extent `WindowResizeEvent` is the Application minimize signal
(`Application.cpp:215-219`), but the event type itself is pure data.

### Key events — `KeyEvent` (protected base), `KeyPressedEvent`,
`KeyReleasedEvent` — flags: `pure-logic-unit`

`getKeyCode` on the base; `KeyPressedEvent` adds a repeat count
(`KeyEvent.h:10-61`); both `KEYBOARD|INPUT` categories (`KeyEvent.h:17`).
`KeyPressedEvent` with `RB_KEY_ESCAPE` is what Application turns into a close
(`Application.cpp:76-82`), but the event types themselves are pure data.

### Mouse events — `MouseMoveEvent`, `MouseScrolledEvent`,
`MouseButtonEvent` (protected base), `MousePressedEvent`,
`MouseReleasedEvent` — flags: `pure-logic-unit`

Position/offset/button payloads with `MOUSE|INPUT` (or `MOUSE_BUTTON|INPUT`)
categories and `toString` strings (`MouseEvent.h:10-121`). Pure data; the
existing `Tests/src/EventChecks.cpp` already exercises them headlessly through
the Engine-linked Regression target.

## Module: Layer (`Engine/Source/Rubber/Layer`)

Files: `Layer.{h,cpp}`, `LayerStack.{h,cpp}`.

### `Layer` — flags: `pure-logic-unit`

Abstract base: constructor takes a debug name (default "Layer") and stores it
(`Layer.h:12`, `Layer.cpp:5-8`); `getName` returns it (`Layer.h:20-23`);
`onAttach`/`onDetach` are pure virtual, so test subclasses are the intended
use (`Layer.h:15-16`); `onUpdate(float)` and `onImGuiRender()` default to no-ops
(`Layer.h:17-18`). `Layer.cpp` includes `Rubber/Event/Event.h` but uses
nothing from it (`Layer.cpp:3`), so the TU's only real needs are pch and its
own header. `ImGuiLayer` (outside these trees) subclasses it and is the only
Layer subtype with GL/ImGui edges.

What constructing it requires today: a concrete subclass implementing
`onAttach`/`onDetach`; nothing else.

TU verdict: compile `Layer.cpp` in UnitTests.

### `LayerStack` — flags: `pure-logic-unit`

Existing user-observable behavior: ordinary layers always precede overlays;
`pushLayer` inserts at the ordinary/overlay boundary and advances it,
`pushOverlay` appends (`LayerStack.cpp:18-22,35-38`); `popLayer` removes only
from the ordinary partition and `popOverlay` only from overlays, each a no-op
for absent or wrong-partition pointers (`LayerStack.cpp:24-33,40-47`);
popping transfers lifetime to the caller without deleting
(`LayerStack.h:21-33`); destruction deletes exactly the layers still stored
(`LayerStack.cpp:10-16`); `begin`/`end` expose forward traversal
(`LayerStack.cpp:50-56`), which `Application::runFrame` and `attachAll` use
(`Application.cpp:168,240`). Data is a plain `std::vector<Layer*>`
(`LayerStack.h:44`); the include chain is `LayerStack.h` → `Utility.h` only.

What constructing it requires today: default construction; nothing else.

TU verdict: compile `LayerStack.cpp` in UnitTests. The existing
`Tests/src/LayerPartitionCheck.cpp` already asserts all of the above behavior
headlessly (through the Engine-linked Regression target), so the baseline
suite has a ready oracle to port.

## TU list for a no-Engine-link UnitTests baseline

Compile these six Engine TUs directly (all start with `#include <pch.h>`, which
resolves from the `Engine/Source` include dir already present in
`Tests/Build-UnitTests.lua:16`):

1. `Engine/Source/Rubber/Event/EventManager.cpp`
2. `Engine/Source/Rubber/Layer/Layer.cpp`
3. `Engine/Source/Rubber/Layer/LayerStack.cpp`
4. `Engine/Source/Rubber/Core/Log.cpp`
5. `Engine/Source/Rubber/Core/GameLoopTimer.cpp`
6. `Engine/Source/Rubber/Core/Utility.cpp`

Exclude `Engine/Source/Rubber/Core/Application.cpp` (needs-GL, needs-ImGui,
singleton-locked; drags Platform/Windows, Renderer, imGui, Input, Resources
into any compile, `Application.cpp:4-31`).

Build settings UnitTests must add beyond its current lua
(`Tests/Build-UnitTests.lua`): include dirs
`Engine/RBVendor/spdlogs/include` and `Engine/RBVendor/glfw/include`; defines
`WINDOW_64_API` and `GLFW_INCLUDE_NONE`; Debug parity also wants
`RB_DEBUG`+`RB_ENABLE_ASSERTS` so assert-guarded contracts (duplicate handler
names, missing names, second Application) behave as in Engine Debug. Link
impact is `winmm.lib` only, and pch.h's pragma supplies it under
`WINDOW_64_API` (`pch.h:31-32`). spdlog is header-only here (no spdlog
library in Engine links, `Build-Engine.lua:39-46`).

Runtime setup the runner needs: one call to `Rubber::Logger::Init()` before
any assert-guarded path in Debug configurations.

## Flag tally

- `pure-logic-unit`: `GameLoopTimer`, `Logger`, `RandomEngine`, `HashFunc`,
  `stringHash`, Utility aliases, `EventType`, `EventCategory`, `toUnderType`,
  `Event`, `EventBus<T>`, `EventManager`, all 12 concrete event classes and
  both event base classes, `Layer`, `LayerStack`.
- `needs-GL`: `Application` only, in these trees.
- `needs-ImGui`: `Application` only, in these trees.
- `singleton-locked`: `Application` (private-ish instance via `s_Instance`
  assert, never cleared) and `EventManager` (private ctor, process-lifetime
  `create()` cache). `EventBus<T`> is process-global static state but not
  construction-locked; `Logger` is a Meyers singleton behind `Init`, not a
  test lock.
- `needs-native-dialog`: none. No dialog API call exists anywhere under
  `Engine/Source/Rubber` (grep for `MessageBox|GetOpenFileName|DialogBox|
  IFileOpenDialog|shellapi` over the whole Rubber tree: no matches).

## Classification coverage

Public types in the three trees, all classified above: Core (9):
`Application`, `GameLoopTimer`, `Logger`, `RandomEngine`/`RandomEngineImpl`,
`HashFunc`, `stringHash`, Utility aliases (`Vector`, `Deque`, `Ref`, `Scope`,
`weakRef`, `makeRef`, `makeScope`), `Core.h`/`Assert.h` macro surface,
`EntryPoint.h` main surface. Event (16): `EventType`, `EventCategory`,
`toUnderType`, `Event`, `EventBus<T>`, `EventManager`, `WindowResizeEvent`,
`WindowCloseEvent`, `ViewPortResizeEvent`, `AppTickEvent`, `AppUpdateEvent`,
`AppRenderEvent`, `KeyEvent`, `KeyPressedEvent`, `KeyReleasedEvent`,
`MouseMoveEvent`, `MouseScrolledEvent`, `MouseButtonEvent`,
`MousePressedEvent`, `MouseReleasedEvent`. Layer (2): `Layer`, `LayerStack`.

PASS: every public type in `Rubber/Core`, `Rubber/Event`, `Rubber/Layer` is
classified.

---

pwd: `C:\Users\yhl20\.t3\worktrees\GameFramework-rb-test-layering`
git branch --show-current: `test/rb-baseline-layering`
git rev-parse HEAD: `494c32c4ec085f83e78b7ba21b258f781a25b567`
