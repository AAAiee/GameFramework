#pragma once

#include <imgui.h>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Rubber {

    /** One submitted Dear ImGui item from the current frame's test-engine hooks. */
    struct UiItem {
        ImGuiID id = 0;
        ImVec2 min{};
        ImVec2 max{};
        std::string label;
        std::string window;
        std::string path;
        int statusFlags = 0;
    };

    /** Process-wide catalog of the live ImGui context's items.
     * Production leaves the hook flag false. Regression calls enable() after
     * CreateContext. attachToContext() exists so Editor.exe and Regression.exe
     * both link the hook symbols ImGui references when IMGUI_ENABLE_TEST_ENGINE
     * is defined.
     */
    class ItemProbe {
    public:
        static void attachToContext();
        static void enable();
        static void disable();
        static void beginFrame();
        static void bindLabel(ImGuiID id, std::string_view label);
        static std::optional<UiItem> find(std::string_view query, int occurrence = 0);
        static std::vector<UiItem> snapshot();
        static void dump(std::ostream& out);
        static ImVec2 center(const UiItem& item);
    };

}
