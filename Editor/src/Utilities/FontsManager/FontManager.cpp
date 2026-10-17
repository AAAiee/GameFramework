#include "FontManager.h"
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// Validate before atlas insertion: ImGui defers parsing until Build(), where
// an invalid font would otherwise assert rather than give the caller an error.
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>

namespace {
    std::unordered_map<std::string, ImFont*> s_FontsMap;
}

void FontManager::loadFontFromTTF(FontInfo info)
{
    const auto fail = [&info](const char* reason) {
        throw std::runtime_error("Cannot load font '" + info.path.string() + "': " + reason);
    };
    auto& io = ImGui::GetIO();
    if (io.Fonts->Locked) fail("atlas is locked during a frame");
    if (!std::isfinite(info.size) || info.size <= 0.0f) fail("size must be finite and positive");

    std::ifstream file(info.path, std::ios::binary | std::ios::ate);
    if (!file) fail("cannot open file");
    const auto length = file.tellg();
    if (length < 12 || length > (std::numeric_limits<int>::max)()) fail("invalid file size");
    std::vector<unsigned char> bytes(static_cast<size_t>(length));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        fail("cannot read file");
    // stb_truetype, also used by ImGui, expects a complete trusted font file.
    const int offset = stbtt_GetFontOffsetForIndex(bytes.data(), 0);
    stbtt_fontinfo parsed{};
    if (offset < 0 || !stbtt_InitFont(&parsed, bytes.data(), offset)) fail("unsupported or invalid font");

    // Let ImGui copy the validated bytes using its allocator and own that copy.
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(bytes.data(), static_cast<int>(bytes.size()), info.size, &config);
    if (!font) fail("atlas insertion failed");
    s_FontsMap.insert_or_assign(info.name, font);
}

ImFont* FontManager::getFont(std::string_view fontName)
{
    const auto found = s_FontsMap.find(std::string(fontName));
    return found == s_FontsMap.end() ? nullptr : found->second;
}

bool FontManager::setDefaultFont(std::string_view fontName)
{
    if (ImFont* font = getFont(fontName)) {
        ImGui::GetIO().FontDefault = font;
        return true;
    }
    return false;
}
