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
`Position`, `Rotation`, and `Scale` share `X`/`Y`/`Z` and `##X`/`##Y`/`##Z`, so
scripts select a row by occurrence. `ColorEdit4` submits its own `##X`, so the
Sprite color drag runs before Transform is added.
ColorEdit4's preview square and BeginCombo's row skip `ITEM_INFO`.
`EditorUi::colorEdit4` and `EditorUi::beginCombo` bind `Color Picker` and
`Projection` to the id ImGui submits for the clickable item.
Production `Editor.exe` never calls `ItemProbe::enable()`, so the bind is a
no-op there.

## Operations

- `click(path)` / `click(path, button, occurrence)` / `clickPoint(pos)`
- `rightClickBlank(window)`
- `drag(path, delta, occurrence)` / `dragPoint(start, delta)`
- `chord(key)` overlay Ctrl + `KeyPressedEvent`
- `typeText(text)` / `openCombo(path)`
- `beginDialogCancel()` for a native dialog on the private desktop

## Completeness gate

1. Static: `Tests/Windows/scan-ui-catalog.ps1` checks `Editor/src` in both
   directions. A widget label missing from `kUiCatalog` fails, and a catalog id
   with no `ImGui::` or `EditorUi::` literal fails. `Orthographic` and
   `Perspective` come from `ImGui::Selectable(projectionTypeName[i])`, and
   `ItemProbe` tags `##Splitter` from the dock geometry, so those three ids skip
   the literal rule. Hosted CI runs the scan in its own job with a 5 s cap.
2. Runtime: after opening each menu/popup, `ItemProbe` must contain that path.
3. Display host: `Tests/Windows/check-ui-coverage.ps1 <evidence>` unions
   `DRIVEN: ` lines from the evidence logs and fails for a catalog id that is
   neither driven nor a window seen in the item dump. It is not a hosted job.
   Every catalog id must be driven, so there is no pending list.
4. Journey: the catalog is split across six scripts. `ui-hierarchy` creates and
   deletes an entity and dumps the item tree. `ui-inspector` drives Add
   Component, Transform, Sprite color, Camera, Projection, and Visibility.
   `ui-files` drives File New/Open/Save As and Ctrl+N/O/S. `ui-viewport` drags
   the dock splitter and reads the Setting window. `ui-exit` and `ui-escape`
   each close the app in their own process. `drive editor-ui` runs the first
   four as a sequential matrix, then `drive ui-exit` and `drive ui-escape` run
   separately. `verify-rb-editor` feature files name those selectors.
   `verified-unreachable` is not a legal status for a catalogued entry.
5. Hosted CI runs `UnitTests` on every push and pull request.

`IMGUI_ENABLE_TEST_ENGINE` is a Premake define on every target that compiles
or includes Dear ImGui, so the imgui submodule stays clean.
