#include "FontAtlasCheck.h"
#include "Utilities/FontsManager/FontManager.h"
#include <chrono>
#include <fstream>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    struct Context {
        ImGuiContext* previous = ImGui::GetCurrentContext();
        ImGuiContext* owned = ImGui::CreateContext();
        Context() { ImGui::SetCurrentContext(owned); }
        ~Context() {
            ImGui::DestroyContext(owned);
            ImGui::SetCurrentContext(previous);
        }
    };

    struct Files {
        std::filesystem::path directory = std::filesystem::current_path() /
            ("font-atlas-check-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Files() { require(std::filesystem::create_directory(directory), "Cannot create font check working directory"); }
        ~Files() {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }
    };
}

namespace Rubber {
    void checkFontAtlas() {
        Context context;
        Files files;
        auto& io = ImGui::GetIO();
        auto* atlas = io.Fonts;
        const auto regular = std::filesystem::path("assets/fonts/OpenSans/static/OpenSans-Regular.ttf");
        const auto bold = std::filesystem::path("assets/fonts/OpenSans/static/OpenSans-Bold.ttf");
        ImFont* initial = atlas->AddFontDefault();
        require(initial != nullptr, "Cannot insert initial default font");
        FontManager::loadFontFromTTF({ "F11-regular", regular, 18.0f });
        ImFont* first = atlas->Fonts.back();
        require(first != initial && FontManager::getFont("F11-regular") == first,
            "First named load did not retain its actual atlas pointer");
        ImFont* between = atlas->AddFontDefault();
        FontManager::loadFontFromTTF({ "F11-bold", bold, 21.0f });
        ImFont* second = atlas->Fonts.back();
        atlas->AddFontDefault();
        require(second != between && second != first && FontManager::getFont("F11-bold") == second,
            "Second named load used a counted index instead of the inserted pointer");
        require(FontManager::getFont("F11-regular") == first, "Other insertions shifted the first named result");

        const std::string extended = "F11-regular-extra";
        FontManager::loadFontFromTTF({ extended, bold, 15.0f });
        ImFont* suffix = atlas->Fonts.back();
        const std::string_view sliced(extended.data(), std::string_view("F11-regular").size());
        require(FontManager::getFont(sliced) == first, "Sliced lookup selected the suffix-bearing name");
        const char bounded[] = { 'F', '1', '1', '-', 'b', 'o', 'l', 'd' };
        require(FontManager::getFont(std::string_view(bounded, sizeof(bounded))) == second,
            "Nonterminated name did not resolve exactly");
        require(FontManager::getFont(extended) == suffix, "Sliced lookup changed the full name");
        require(FontManager::setDefaultFont(sliced) && io.FontDefault == first,
            "Default selection did not use the named atlas pointer");
        require(FontManager::setDefaultFont("F11-bold") && io.FontDefault == second,
            "Default selection did not select the second named pointer");
        const int count = atlas->Fonts.Size;
        for (int attempt = 0; attempt < 2; ++attempt) {
            require(FontManager::getFont("F11-unknown") == nullptr, "Unknown lookup returned a font");
            require(FontManager::getFont(std::string_view{}) == nullptr, "Empty lookup returned a font");
            require(!FontManager::setDefaultFont("F11-unknown") && io.FontDefault == second,
                "Unknown default selection changed FontDefault");
        }
        require(atlas->Fonts.Size == count, "Lookup mutated the atlas");

        const auto expectFailure = [&](const char* name, const std::filesystem::path& path, ImFont* previous) {
            const int before = atlas->Fonts.Size;
            bool failed = false;
            try {
                FontManager::loadFontFromTTF({ name, path, 18.0f });
            } catch (const std::runtime_error& error) {
                failed = true;
                require(std::string(error.what()).find(path.string()) != std::string::npos,
                    "Load failure omitted the path");
            }
            require(failed, "Unloadable font was reported as a successful load");
            require(FontManager::getFont(name) == previous, "Failed load changed registration");
            require(atlas->Fonts.Size == before && io.FontDefault == second,
                "Failed load mutated the atlas or default selection");
        };
        expectFailure("F11-missing", files.directory / "missing.ttf", nullptr);
        const auto empty = files.directory / "empty.ttf";
        { std::ofstream out(empty, std::ios::binary); require(bool(out), "Cannot create empty font fixture"); }
        expectFailure("F11-empty", empty, nullptr);
        const auto invalid = files.directory / "invalid.ttf";
        {
            std::ofstream out(invalid, std::ios::binary);
            out << "This is plain text, not a TrueType font file.";
            require(bool(out), "Cannot write invalid font fixture");
        }
        expectFailure("F11-invalid", invalid, nullptr);
        expectFailure("F11-regular", invalid, first);
        FontManager::loadFontFromTTF({ "F11-unknown", regular, 16.0f });
        require(FontManager::getFont("F11-unknown") == atlas->Fonts.back(),
            "Unknown lookup prevented later registration");
        require(atlas->Build(), "Real atlas could not build the loaded fonts");
        require(first->IsLoaded() && second->IsLoaded(), "Named pointers did not refer to built fonts");
        require(FontManager::getFont(sliced) == first && io.FontDefault == second,
            "Atlas build changed named/default selection");
        // Do not use these borrowed pointers after Context destroys the atlas.
    }
}
