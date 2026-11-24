#include "pch.h"
#include "Rubber/imGui/ItemProbe.h"

#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <unordered_map>

namespace {
    bool g_Enabled = false;
    std::vector<Rubber::UiItem> g_Items;
    std::unordered_map<ImGuiID, std::size_t> g_Index;

    std::string makePath(const std::string& window, const std::string& label) {
        if (window.empty()) return label;
        if (label.empty() || label == window) return window;
        return window + "/" + label;
    }

    bool matches(const Rubber::UiItem& item, std::string_view query) {
        if (item.path == query || item.label == query) return true;
        if (item.window == query) return true;
        const std::string suffix = "/" + std::string(query);
        return item.path.size() >= suffix.size()
            && item.path.compare(item.path.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    float area(const Rubber::UiItem& item) {
        const float w = item.max.x - item.min.x;
        const float h = item.max.y - item.min.y;
        if (w <= 0.0f || h <= 0.0f) return 0.0f;
        return w * h;
    }

    /** ColorEdit4's preview is ColorButton("##ColorButton") with no ItemInfo.
     * The user-facing label is TextEx only. Tag the square beside ##W so
     * find("Color Picker") hits the control a user clicks.
     */
    void synthesizeLabels() {
        for (auto& item : g_Items) {
            if (!item.label.empty()) continue;
            const float width = item.max.x - item.min.x;
            const float height = item.max.y - item.min.y;

            if (width >= 12.0f && height >= 12.0f && width <= 40.0f && height <= 40.0f
                && std::fabs(width - height) <= 8.0f) {
                for (const auto& other : g_Items) {
                    if (other.label != "##W" || other.window != item.window) continue;
                    if (item.min.y > other.max.y || item.max.y < other.min.y) continue;
                    if (item.min.x + 1.0f < other.max.x) continue;
                    if (item.min.x > other.max.x + 16.0f) continue;
                    item.label = "Color Picker";
                    item.path = makePath(item.window, item.label);
                    break;
                }
            }

            if (item.label.empty() && width >= 80.0f && height >= 16.0f && height <= 32.0f) {
                for (const auto& size : g_Items) {
                    if ((size.label != "Size" && size.label != "FOVY")
                        || size.window != item.window) continue;
                    if (std::fabs(item.min.x - size.min.x) > 2.0f) continue;
                    if (item.max.y > size.min.y) continue;
                    if (size.min.y - item.max.y > 16.0f) continue;
                    item.label = "Projection";
                    item.path = makePath(item.window, item.label);
                    break;
                }
            }
        }
    }
}

void Rubber::ItemProbe::attachToContext() {
}

void Rubber::ItemProbe::enable() {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) return;
    ctx->TestEngineHookItems = true;
    g_Enabled = true;
}

void Rubber::ItemProbe::disable() {
    if (ImGuiContext* ctx = ImGui::GetCurrentContext())
        ctx->TestEngineHookItems = false;
    g_Enabled = false;
    g_Items.clear();
    g_Index.clear();
}

void Rubber::ItemProbe::beginFrame() {
    if (!g_Enabled) return;
    g_Items.clear();
    g_Index.clear();
}

std::optional<Rubber::UiItem> Rubber::ItemProbe::find(std::string_view query, int occurrence) {
    synthesizeLabels();
    std::vector<const UiItem*> hits;
    hits.reserve(8);
    for (const auto& item : g_Items) {
        if (area(item) < 1.0f) continue;
        if (matches(item, query)) hits.push_back(&item);
    }
    std::sort(hits.begin(), hits.end(), [](const UiItem* a, const UiItem* b) {
        const float aa = area(*a);
        const float ba = area(*b);
        if (aa != ba) return aa < ba;
        if (a->min.y != b->min.y) return a->min.y < b->min.y;
        if (a->min.x != b->min.x) return a->min.x < b->min.x;
        return a->id < b->id;
    });
    if (occurrence < 0 || static_cast<std::size_t>(occurrence) >= hits.size())
        return std::nullopt;
    return *hits[static_cast<std::size_t>(occurrence)];
}

std::vector<Rubber::UiItem> Rubber::ItemProbe::snapshot() {
    synthesizeLabels();
    return g_Items;
}

void Rubber::ItemProbe::dump(std::ostream& out) {
    synthesizeLabels();
    out << "UI items (" << g_Items.size() << ")\n";
    for (const auto& item : g_Items) {
        out << "  " << item.path
            << " label='" << item.label
            << "' window='" << item.window
            << "' id=" << std::hex << item.id << std::dec
            << " rect=(" << item.min.x << "," << item.min.y
            << ")-(" << item.max.x << "," << item.max.y << ")\n";
    }
}

ImVec2 Rubber::ItemProbe::center(const UiItem& item) {
    return ImVec2((item.min.x + item.max.x) * 0.5f, (item.min.y + item.max.y) * 0.5f);
}

void ImGuiTestEngineHook_ItemAdd(ImGuiContext* ctx, const ImRect& bb, ImGuiID id) {
    if (!g_Enabled || !ctx || id == 0) return;
    Rubber::UiItem item;
    item.id = id;
    item.min = bb.Min;
    item.max = bb.Max;
    if (ctx->CurrentWindow) item.window = ctx->CurrentWindow->Name;
    const float width = bb.Max.x - bb.Min.x;
    const float height = bb.Max.y - bb.Min.y;
    if ((width > 0.0f && width <= 6.0f && height >= 40.0f)
        || (height > 0.0f && height <= 6.0f && width >= 40.0f))
        item.label = "##Splitter";
    item.path = makePath(item.window, item.label);
    auto found = g_Index.find(id);
    if (found != g_Index.end()) {
        auto& existing = g_Items[found->second];
        existing.min = item.min;
        existing.max = item.max;
        if (existing.window.empty()) existing.window = item.window;
        if (existing.label.empty()) existing.label = item.label;
        existing.path = makePath(existing.window, existing.label);
        return;
    }
    g_Index[id] = g_Items.size();
    g_Items.push_back(std::move(item));
}

void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label, ImGuiItemStatusFlags flags) {
    if (!g_Enabled || !ctx || id == 0) return;
    std::string text = label ? label : "";
    auto found = g_Index.find(id);
    if (found == g_Index.end()) {
        ImGuiTestEngineHook_ItemAdd(ctx, ImRect(), id);
        found = g_Index.find(id);
        if (found == g_Index.end()) return;
    }
    auto& item = g_Items[found->second];
    if (!(text.empty() && !item.label.empty()))
        item.label = text;
    item.statusFlags = flags;
    if (item.window.empty() && ctx->CurrentWindow)
        item.window = ctx->CurrentWindow->Name;
    item.path = makePath(item.window, item.label);
}

void ImGuiTestEngineHook_Log(ImGuiContext*, const char*, ...) {
}

const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*, ImGuiID id) {
    auto found = g_Index.find(id);
    if (found == g_Index.end()) return "";
    return g_Items[found->second].label.c_str();
}
