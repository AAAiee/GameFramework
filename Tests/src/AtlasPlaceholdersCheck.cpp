#include "pch.h"
#include "AtlasPlaceholdersCheck.h"
#include "Rubber/Resources/AssetManager.h"
#include "Rubber/Resources/AnimationResources/AnimationAtlas.h"
#include "Rubber/Renderer/Texture.h"
#include <glad/glad.h>
#include <cstring>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    struct Files {
        std::filesystem::path directory = std::filesystem::current_path() /
            ("atlas-placeholders-check-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Files() { require(std::filesystem::create_directory(directory), "Cannot create atlas fixture directory"); }
        ~Files() {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }
    };

    // Complete 2x2 RGBA PNGs, solid red/green/blue variants. Each row is eight
    // bytes: these fixtures do not depend on F13's RGB row-alignment repair.
    // Paths below are owned, terminated strings at the existing library boundary.
    const std::vector<unsigned char> images[] = {
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
            0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00,
            0x11, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x78, 0x2e, 0xa8, 0xfe,
            0x1f, 0x84, 0x19, 0x60, 0x0c, 0x00, 0x4b, 0x8e, 0x08, 0x79, 0xbc, 0x68,
            0x93, 0x22, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
            0x60, 0x82
        },
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
            0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00,
            0x11, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x10, 0x3f, 0x6a, 0xfb,
            0x1f, 0x84, 0x19, 0x60, 0x0c, 0x00, 0x47, 0x12, 0x08, 0x61, 0x0a, 0x06,
            0xf1, 0xd5, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
            0x60, 0x82
        },
        {
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
            0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00,
            0x11, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xd0, 0xf4, 0xbc, 0xfd,
            0x1f, 0x84, 0x19, 0x60, 0x0c, 0x00, 0x4c, 0x9a, 0x09, 0x31, 0x23, 0x81,
            0x15, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
            0x60, 0x82
        }
    };
    using Pixel = std::array<unsigned char, 4>;
    constexpr Pixel colors[] = { {231, 17, 39, 255}, {23, 197, 61, 255}, {41, 73, 219, 255} };

    void checkPixels(const Rubber::Ref<Rubber::Texture2D>& texture, bool missing, unsigned frame) {
        require(texture && texture->getType() == Rubber::AssetType::Texture2D, "Frame is not a texture");
        const int side = missing ? 1 : 2;
        require(texture->getWidth() == side && texture->getHeight() == side,
            "Imported frame dimensions changed");
        std::array<Pixel, 4> pixels{};
        glGetTextureImage(texture->getTextureID(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
            static_cast<GLsizei>(sizeof(pixels)), pixels.data());
        require(glGetError() == GL_NO_ERROR, "Texture import/readback produced a GL error");
        Pixel expected = colors[frame];
        if (missing) {
            // Preserve the existing fallback's native uint32_t pixel value.
            const uint32_t fallbackPixel = 0xFF00FFFF;
            std::memcpy(expected.data(), &fallbackPixel, sizeof(fallbackPixel));
        }
        for (int i = 0; i < side * side; ++i)
            require(pixels[i] == expected, "Frame texture pixels do not match the declared position");
    }
}

namespace Rubber {
    void checkAtlasPlaceholders() {
        require(glGetString(GL_VERSION) != nullptr && GLAD_GL_VERSION_4_5,
            "Atlas check requires a current OpenGL 4.5 context");
        Files files;
        Ref<Texture2D> fallback;
        // All eight masks include missing first, middle, last, all, multiple,
        // and no missing images. Every case uses the actual AssetManager ->
        // AssetImporter -> TextureImporter -> AnimationAtlas path.
        for (unsigned mask = 0; mask < 8; ++mask) {
            const auto folder = files.directory / ("case-" + std::to_string(mask));
            require(std::filesystem::create_directory(folder), "Cannot create atlas case directory");
            for (unsigned frame = 0; frame < 3; ++frame) {
                if (mask & (1u << frame)) continue;
                std::ofstream output(folder / (std::to_string(frame + 1) + ".png"), std::ios::binary);
                output.write(reinterpret_cast<const char*>(images[frame].data()),
                    static_cast<std::streamsize>(images[frame].size()));
                output.close();
                require(bool(output), "Cannot write RGBA PNG fixture");
            }
            AssetManager manager;
            AnimationResourcesMetaData metadata("F12-atlas", AssetType::AnimationAtlas, folder.string(), 3);
            const auto handle = manager.loadAsset(metadata);
            require(handle == AssetHandle(metadata.name) && manager.isAssetLoaded(handle),
                "Atlas was not registered under its declared handle");
            const auto atlas = std::dynamic_pointer_cast<AnimationAtlas>(manager.getAsset(handle));
            require(bool(atlas), "Animation import did not return an atlas");
            const auto& registered = dynamic_cast<const AnimationResourcesMetaData&>(manager.getAssetMetadata(handle));
            require(registered.frameCount == metadata.frameCount && registered.filepath == folder,
                "Atlas registration changed animation metadata");
            require(atlas->m_Frames.size() == metadata.frameCount,
                "Atlas must retain exactly the declared frameCount entries");
            for (unsigned frame = 0; frame < metadata.frameCount; ++frame) {
                const bool missing = (mask & (1u << frame)) != 0;
                const auto name = folder.filename().string() + std::to_string(frame + 1);
                const AssetHandle frameHandle(name);
                require(atlas->m_Frames[frame] == frameHandle && manager.isAssetLoaded(frameHandle),
                    "Atlas frame handle was omitted, duplicated or shifted");
                const auto& frameMetadata = manager.getAssetMetadata(frameHandle);
                require(frameMetadata.name == name && frameMetadata.type == AssetType::Texture2D &&
                    frameMetadata.filepath == folder / (std::to_string(frame + 1) + ".png"),
                    "Frame registration lost its name, type or declared path");
                const auto texture = atlas->getTextureAtIndex(frame);
                require(texture == std::dynamic_pointer_cast<Texture2D>(manager.getAsset(frameHandle)),
                    "Atlas lookup did not resolve the registered frame asset");
                checkPixels(texture, missing, frame);
                if (missing) {
                    if (!fallback) fallback = texture;
                    require(texture == fallback, "Missing slots do not share the existing fallback texture");
                } else {
                    require(texture != fallback, "Present image resolved to the fallback");
                    for (unsigned previous = 0; previous < frame; ++previous)
                        require(texture != atlas->getTextureAtIndex(previous), "Present frames lost their distinct imports");
                }
            }
            // atlas and its present-image textures die before their borrowed manager,
            // while the Regression Application still owns a live GL context.
        }
        require(bool(fallback), "No fallback was exercised");
    }
}
