#include "pch.h"
#include "GLBoundariesCheck.h"
#include "Platform/OpenGL/GLShader.h"
#include "Platform/OpenGL/GLTexture2D.h"
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    struct Files {
        std::filesystem::path directory = std::filesystem::current_path() /
            ("gl-boundaries-check-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Files() { require(std::filesystem::create_directory(directory), "Cannot create graphics fixture directory"); }
        ~Files() {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }
    };

    // Isolate upload and readback from ambient row strides, skips and PBOs.
    // Readback always packs tightly, regardless of the unpack alignment tested.
    struct PixelState {
        static constexpr GLenum fields[] = {
            GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_PIXELS, GL_PACK_SKIP_ROWS,
            GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_ROWS
        };
        GLint values[8]{}, packBuffer = 0, unpackBuffer = 0, program = 0;
        PixelState() {
            for (unsigned i = 0; i < 8; ++i) {
                glGetIntegerv(fields[i], &values[i]);
                glPixelStorei(fields[i], i == 0 || i == 4 ? 1 : 0);
            }
            glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
            glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
            glGetIntegerv(GL_CURRENT_PROGRAM, &program);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
        ~PixelState() {
            for (unsigned i = 0; i < 8; ++i) glPixelStorei(fields[i], values[i]);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, packBuffer);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
            glUseProgram(program);
        }
    };

    void checkAlignment(GLint expected) {
        GLint actual = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &actual);
        require(actual == expected, "Texture upload did not preserve unpack alignment");
    }

    std::vector<unsigned char> pixels(int width, int channels, int seed) {
        std::vector<unsigned char> result(width * 3 * channels);
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = static_cast<unsigned char>((i * 37 + seed) % 251);
        return result;
    }

    // Uncompressed true-color TGA, top-origin on disk, BGR(A) channel order.
    // Expected GL pixels are bottom-row first, exercising stb's vertical flip.
    void writeImage(const std::filesystem::path& path, int width, int channels,
        const std::vector<unsigned char>& bottomFirst) {
        std::array<unsigned char, 18> header{};
        header[2] = 2;
        header[12] = static_cast<unsigned char>(width);
        header[14] = 3;
        header[16] = static_cast<unsigned char>(channels * 8);
        header[17] = static_cast<unsigned char>(0x20 | (channels == 4 ? 8 : 0));
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(header.data()), header.size());
        for (int y = 2; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                const auto* p = bottomFirst.data() + (y * width + x) * channels;
                output.put(static_cast<char>(p[2]));
                output.put(static_cast<char>(p[1]));
                output.put(static_cast<char>(p[0]));
                if (channels == 4) output.put(static_cast<char>(p[3]));
            }
        }
        output.close();
        require(bool(output), "Cannot write graphics image fixture");
    }

    void checkPixels(const Rubber::Texture2D& texture, int channels,
        const std::vector<unsigned char>& expected) {
        std::vector<unsigned char> actual(expected.size(), 0);
        glGetTextureImage(texture.getTextureID(), 0, channels == 3 ? GL_RGB : GL_RGBA,
            GL_UNSIGNED_BYTE, static_cast<GLsizei>(actual.size()), actual.data());
        require(glGetError() == GL_NO_ERROR, "Graphics upload/readback produced a GL error");
        require(actual == expected, "Texture rows, channel values or orientation changed");
    }
}

namespace Rubber {
    void checkGLBoundaries() {
        require(glGetString(GL_VERSION) && GLAD_GL_VERSION_4_5,
            "Graphics boundary check requires a current OpenGL 4.5 context");
        require(glGetError() == GL_NO_ERROR, "GL error before graphics boundary check");
        PixelState state;
        Files files;
        for (const int channels : {3, 4}) {
            for (const int width : {1, 3, 5}) {
                const auto original = pixels(width, channels, 11);
                const auto replacement = pixels(width, channels, 103);
                const auto path = files.directory / (std::to_string(width) + "-" + std::to_string(channels) + ".tga");
                writeImage(path, width, channels, original);
                for (const GLint alignment : {1, 4, 8}) {
                    const auto intendedPath = path.string();
                    std::string backing = intendedPath + ".unused-suffix";
                    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
                    GLTexture2D texture(std::string_view(backing.data(), intendedPath.size()));
                    backing.assign(backing.size(), 'x'); // texture retains no borrowed path
                    checkAlignment(alignment);
                    require(texture.getWidth() == width && texture.getHeight() == 3,
                        "Sliced texture path did not load intended dimensions");
                    checkPixels(texture, channels, original);
                    // Padding keeps a broken strided upload within allocated storage;
                    // only the exact tightly packed byte count is passed to production.
                    auto upload = replacement;
                    upload.resize(upload.size() + 32, 0xee);
                    texture.setData(upload.data(), static_cast<uint32_t>(replacement.size()));
                    checkAlignment(alignment);
                    checkPixels(texture, channels, replacement);
                }
            }
        }
        // The size-only constructor retains RGBA storage and accepts tight rows too.
        GLTexture2D empty(3, 3);
        auto rgba = pixels(3, 4, 59);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
        auto rgbaUpload = rgba;
        rgbaUpload.resize(rgbaUpload.size() + 32, 0xee);
        empty.setData(rgbaUpload.data(), static_cast<uint32_t>(rgba.size()));
        checkAlignment(8);
        checkPixels(empty, 4, rgba);

        const auto shaderPath = (files.directory / "bounded.glsl").string();
        std::ofstream source(shaderPath, std::ios::binary);
        source << "#type vertex\n#version 450 core\n"
            "void main() { gl_Position = vec4(0,0,0,1); }\n"
            "#type fragment\n#version 450 core\n"
            "layout(location=0) out vec4 color;\n"
            "uniform float u_Value;\nuniform float u_ValueSuffix;\n"
            "void main() { color = vec4(u_Value, u_ValueSuffix, 0, 1); }\n";
        source.close();
        require(bool(source), "Cannot write bounded shader fixture");
        std::string shaderBacking = shaderPath + ".unused-suffix";
        GLShader shader(std::string_view(shaderBacking.data(), shaderPath.size()));
        shaderBacking.assign(shaderBacking.size(), 'x');
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.getProgramId(), GL_LINK_STATUS, &linked);
        require(linked == GL_TRUE && shader.getName() == "bounded", "Sliced shader path did not load intended program");
        const GLint shortLocation = glGetUniformLocation(shader.getProgramId(), "u_Value");
        const GLint longLocation = glGetUniformLocation(shader.getProgramId(), "u_ValueSuffix");
        require(shortLocation >= 0 && longLocation >= 0 && shortLocation != longLocation,
            "Fixture uniforms must both be active and distinct");
        shader.setFloat("u_ValueSuffix", 0.75f);
        std::string nameBacking = "u_ValueSuffix";
        shader.setFloat(std::string_view(nameBacking.data(), 7), 0.25f);
        nameBacking.assign("u_ValueOtherUnusedSuffix");
        shader.setFloat(std::string_view(nameBacking.data(), 7), 0.5f); // cached bounded key
        GLfloat shortValue = 0, longValue = 0;
        glGetUniformfv(shader.getProgramId(), shortLocation, &shortValue);
        glGetUniformfv(shader.getProgramId(), longLocation, &longValue);
        require(shortValue == 0.5f && longValue == 0.75f,
            "Sliced uniform name or cached lookup addressed suffix bytes");
        require(glGetError() == GL_NO_ERROR, "Shader boundary check produced a GL error");
    }
}
