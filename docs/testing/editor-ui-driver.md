# Editor UI driver

Every Editor menu, widget, shortcut, dock splitter, and native dialog is
reachable through one in-process driver. A listed user entry point that the
driver cannot click, type, drag, or chord is a failing completeness check, not
a `verified-unreachable` skip.

## Why

Regression fixtures previously guessed ImGui coordinates, called `saveScene` /
`openScene` without the File menu, injected ViewPort sizes instead of dragging
the dock splitter, and never synthesized GLFW key state. `Ctrl+N` / `Ctrl+O` /
`Ctrl+S` poll `glfwGetKey`. Escape is a `KeyPressedEvent`. Those paths were
unreachable as the user performs them.

## Shapes considered

| Shape | What it is | Why not |
| --- | --- | --- |
| A. Catalog + ItemProbe + EditorDriver | Enable Dear ImGui test-engine item hooks, record rects/labels, click by path, overlay GLFW keys, keep the Win32 dialog driver | **Chosen.** Smallest surface that still drives production widgets. |
| B. Vendor `imgui_test_engine` | Full Dear ImGui test suite | Heavy, still misses GLFW poll and Win32 dialogs. |
| C. Production command registry | Wrap every `MenuItem` in a command object | Extra production API. A registry call is not a menu click. |

## Ownership

| Type | Owner | Notes |
| --- | --- | --- |
| `UiItem` | `ItemProbe` | One per submitted ImGui item this frame. |
| Per-frame item list | `ItemProbe` | Cleared at `NewFrame`. Hooks fill it. |
| Synthetic GLFW keys | `InputOverlay` | `WindowInput` consults it before `glfwGetKey`. |
| Queued mouse/key gestures | `EditorDriver` | Tests only. Applies `ImGuiIO` events between frames. |
| Native `#32770` dialogs | existing Win32 helper | Started before the frame that opens the dialog. |
| Catalog | `Tests/src/UiCatalog.h` | Source of truth for user-facing options. |

Production `Editor.exe` links the hook symbols and never sets
`ImGuiContext::TestEngineHookItems`. Behavior is unchanged.

## Path language

`{window}/{label}` with an optional `#n` occurrence suffix.

Menus and popups exist only after an open gesture. `EditorDriver` clicks
`File` then `New`. Blank-space context menus use `rightClickBlank("Scene Hierarchy")`
then `Create Entity`. Duplicate labels (`X`, `...`, `##Splitter`) disambiguate
by occurrence index. Equal-size hits are top-to-bottom, then left-to-right.
ColorEdit4's preview square has no ItemInfo; ItemProbe tags it `Color Picker`
when it sits beside `##W`. BeginCombo has no ItemInfo; ItemProbe tags the
unlabeled row directly above `Size` or `FOVY` as `Projection`.

## Operations

- `click(path)` / `click(path, button)` / `clickPoint(pos)`
- `rightClickBlank(window)`
- `drag(path, delta)` / `dragPoint(start, delta)`
- `chord(ctrl, key)` overlay + `KeyPressedEvent`
- `key(key)` enqueue `KeyPressedEvent` (Escape)
- native dialog cancel/submit on the private desktop

## Completeness gate

1. Static: `Tests/Windows/scan-ui-catalog.ps1` greps `Editor/src` widget
   literals against `UiCatalog.h` and fails on a missing or extra label.
2. Runtime: after opening each menu/popup, `ItemProbe` must contain that path.
3. Journey: each catalog entry is driven by `Regression.exe editor-ui` or
   `editor-ui-escape`, with a scene, dialog, or window-close observation.
4. `verify-rb-editor` feature files name those selectors. `verified-unreachable`
   is not a legal status for a catalogued entry.

`IMGUI_ENABLE_TEST_ENGINE` is a Premake define on every target that compiles
or includes Dear ImGui, so the imgui submodule stays clean.
