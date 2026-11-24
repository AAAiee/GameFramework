#pragma once

#include <imgui.h>

#include "Rubber/imGui/ItemProbe.h"

namespace Rubber::EditorUi {

    inline bool colorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0) {
        const bool changed = ImGui::ColorEdit4(label, col, flags);
        ImGui::PushID(label);
        ItemProbe::bindLabel(ImGui::GetID("##ColorButton"), label);
        ImGui::PopID();
        return changed;
    }

    inline bool beginCombo(const char* label, const char* preview, ImGuiComboFlags flags = 0) {
        const ImGuiID rowId = ImGui::GetID(label);
        const bool open = ImGui::BeginCombo(label, preview, flags);
        ItemProbe::bindLabel(rowId, label);
        return open;
    }

}
