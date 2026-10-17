#pragma once

#include <imgui.h>
#include <string_view>
#include <string>
#include <filesystem>

struct FontInfo {
    std::string name;
    std::filesystem::path path;
    float size = 0.0f;
};

// Requires the current ImGui context. Registered pointers borrow its atlas:
// never use them after atlas clearing/replacement or context destruction. This
// process-wide registry must only be used within that atlas/context lifetime.
class FontManager {
public:
    // Load a complete trusted TTF/OTF file before a frame starts. Throws
    // std::runtime_error with path context on open/read/parse/insertion failure
    // or invalid size/locked atlas. Failure preserves any existing registration;
    // success replaces the name, leaving previous fonts owned by the atlas.
    static void loadFontFromTTF(FontInfo info);

    // Exact bounded name lookup; unknown names return nullptr without insertion.
    static ImFont* getFont(std::string_view fontName);

    // Unknown names return false and leave FontDefault unchanged.
    static bool setDefaultFont(std::string_view fontName);
};
