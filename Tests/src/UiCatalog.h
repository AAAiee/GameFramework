#pragma once

namespace Rubber {

    /** User-facing Editor options the driver must reach.
     * Labels match production ImGui string literals in Editor/src.
     * scan-ui-catalog.ps1 greps those literals against this table.
     */
    struct UiCatalogEntry {
        const char* path;
        const char* widget;
        const char* reveal;
    };

    inline constexpr UiCatalogEntry kUiCatalog[] = {
        {"File", "BeginMenu", ""},
        {"New", "MenuItem", "File"},
        {"Open...", "MenuItem", "File"},
        {"Save As...", "MenuItem", "File"},
        {"Exit", "MenuItem", "File"},
        {"Create Entity", "MenuItem", "blank:Scene Hierarchy"},
        {"Delete Entity", "MenuItem", "row-context"},
        {"Add Component", "Button", "selection"},
        {"Camera", "MenuItem", "Add Component"},
        {"Sprite Renderer", "MenuItem", "Add Component"},
        {"Transform", "MenuItem", "Add Component"},
        {"Visibility", "MenuItem", "Add Component"},
        {"Remove Component", "MenuItem", "..."},
        {"...", "Button", "selection"},
        {"X", "Button", "Transform"},
        {"Y", "Button", "Transform"},
        {"Z", "Button", "Transform"},
        {"##X", "DragFloat", "Transform"},
        {"##Y", "DragFloat", "Transform"},
        {"##Z", "DragFloat", "Transform"},
        {"##Tag", "InputText", "selection"},
        {"isVisible", "Checkbox", "Visibility"},
        {"Primary", "Checkbox", "Camera"},
        {"IsFixedRatio", "Checkbox", "Camera"},
        {"Projection", "BeginCombo", "Camera"},
        {"Orthographic", "Selectable", "Projection"},
        {"Perspective", "Selectable", "Projection"},
        {"Size", "DragFloat", "Camera"},
        {"NearClip", "DragFloat", "Camera"},
        {"FarClip", "DragFloat", "Camera"},
        {"FOVY", "DragFloat", "Camera"},
        {"Color Picker", "ColorEdit4", "Sprite Renderer"},
        {"##Splitter", "Splitter", "dock"},
        {"Scene Hierarchy", "Begin", ""},
        {"Property Channel", "Begin", ""},
        {"ViewPort", "Begin", ""},
        {"Setting", "Begin", ""},
        {"DockSpace Demo", "Begin", ""},
    };

    inline constexpr const char* kUiShortcuts[] = {
        "Ctrl+N", "Ctrl+O", "Ctrl+S", "Escape"
    };

}
