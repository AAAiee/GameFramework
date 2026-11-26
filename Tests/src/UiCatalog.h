#pragma once

namespace Rubber {

    /** User-facing Editor options the driver must reach.
     * Ids match production ImGui string literals in Editor/src.
     * scan-ui-catalog.ps1 checks both directions.
     */
    enum class UiKind {
        Window, Menu, MenuItem, Button, Checkbox,
        DragFloat, InputText, ColorEdit, Combo, Selectable, Splitter
    };

    enum class UiReveal {
        Shown, FileMenu, HierarchyBlank, HierarchyRow,
        Selection, AddComponent, ComboOpen
    };

    enum class UiObserve {
        Visible, SceneCount, HasComponent, FieldChanged,
        NativeDialog, WindowClosed, ViewportSize
    };

    struct UiEntry {
        const char* id;
        UiKind kind;
        UiReveal reveal;
        UiObserve observe;
    };

    inline constexpr UiEntry kUiCatalog[] = {
        {"File", UiKind::Menu, UiReveal::Shown, UiObserve::Visible},
        {"New", UiKind::MenuItem, UiReveal::FileMenu, UiObserve::SceneCount},
        {"Open...", UiKind::MenuItem, UiReveal::FileMenu, UiObserve::NativeDialog},
        {"Save As...", UiKind::MenuItem, UiReveal::FileMenu, UiObserve::NativeDialog},
        {"Exit", UiKind::MenuItem, UiReveal::FileMenu, UiObserve::WindowClosed},
        {"Create Entity", UiKind::MenuItem, UiReveal::HierarchyBlank, UiObserve::SceneCount},
        {"Delete Entity", UiKind::MenuItem, UiReveal::HierarchyRow, UiObserve::SceneCount},
        {"Add Component", UiKind::Button, UiReveal::Selection, UiObserve::Visible},
        {"Camera", UiKind::MenuItem, UiReveal::AddComponent, UiObserve::HasComponent},
        {"Sprite Renderer", UiKind::MenuItem, UiReveal::AddComponent, UiObserve::HasComponent},
        {"Transform", UiKind::MenuItem, UiReveal::AddComponent, UiObserve::HasComponent},
        {"Visibility", UiKind::MenuItem, UiReveal::AddComponent, UiObserve::HasComponent},
        {"Remove Component", UiKind::MenuItem, UiReveal::Selection, UiObserve::HasComponent},
        {"...", UiKind::Button, UiReveal::Selection, UiObserve::Visible},
        {"X", UiKind::Button, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"Y", UiKind::Button, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"Z", UiKind::Button, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"##X", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"##Y", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"##Z", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"##Tag", UiKind::InputText, UiReveal::Selection, UiObserve::FieldChanged},
        {"isVisible", UiKind::Checkbox, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"Primary", UiKind::Checkbox, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"IsFixedRatio", UiKind::Checkbox, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"Projection", UiKind::Combo, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"Orthographic", UiKind::Selectable, UiReveal::ComboOpen, UiObserve::FieldChanged},
        {"Perspective", UiKind::Selectable, UiReveal::ComboOpen, UiObserve::FieldChanged},
        {"Size", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"NearClip", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"FarClip", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"FOVY", UiKind::DragFloat, UiReveal::AddComponent, UiObserve::FieldChanged},
        {"Color Picker", UiKind::ColorEdit, UiReveal::AddComponent, UiObserve::Visible},
        {"##Splitter", UiKind::Splitter, UiReveal::Shown, UiObserve::ViewportSize},
        {"Scene Hierarchy", UiKind::Window, UiReveal::Shown, UiObserve::Visible},
        {"Property Channel", UiKind::Window, UiReveal::Shown, UiObserve::Visible},
        {"ViewPort", UiKind::Window, UiReveal::Shown, UiObserve::Visible},
        {"Setting", UiKind::Window, UiReveal::Shown, UiObserve::Visible},
        {"DockSpace Demo", UiKind::Window, UiReveal::Shown, UiObserve::Visible},
    };

    inline constexpr const char* kUiShortcuts[] = {
        "Ctrl+N", "Ctrl+O", "Ctrl+S", "Escape"
    };

}
