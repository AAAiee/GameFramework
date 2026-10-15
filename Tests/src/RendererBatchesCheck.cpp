#include "pch.h"
#include "RendererBatchesCheck.h"
#include "Rubber/Renderer/Renderer2D.h"
#include "Rubber/Renderer/Texture.h"
#include "Rubber/Renderer/Subtexture2D.h"
#include "Rubber/Renderer/OrthoCamera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <cmath>
#include <initializer_list>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        // Observe the real GL boundary and forward every call to the driver. No
        // replacement renderer, private RendererData access or statistic-only oracle.
        struct Submission { GLsizei indices; GLint stride; };
        class GLObservation {
        public:
            std::vector<GLsizeiptr> uploads;
            std::vector<Submission> draws;
            PFNGLNAMEDBUFFERDATAPROC upload = glad_glNamedBufferData;
            PFNGLDRAWELEMENTSPROC draw = glad_glDrawElements;
            static GLObservation* active;

            GLObservation() {
                active = this;
                glad_glNamedBufferData = observeUpload;
                glad_glDrawElements = observeDraw;
            }
            ~GLObservation() {
                glad_glNamedBufferData = upload;
                glad_glDrawElements = draw;
                active = nullptr;
            }
            void reset() { uploads.clear(); draws.clear(); }
            void verify(std::initializer_list<unsigned> batches, unsigned quads) const {
                require(draws.size() == batches.size() && uploads.size() == batches.size(),
                    "Unexpected GL draw/upload count (including empty flush)");
                size_t i = 0;
                for (unsigned batch : batches) {
                    require(batch > 0 && batch <= 10000, "Invalid expected batch size");
                    require(draws[i].indices == static_cast<GLsizei>(batch * 6),
                        "GL draw lost quads or exceeded index capacity");
                    require(draws[i].stride > 0 && uploads[i] ==
                        static_cast<GLsizeiptr>(batch) * 4 * draws[i].stride,
                        "GL vertex upload does not match bounded quad count");
                    ++i;
                }
                require(Renderer2D::getQuadCount() == quads
                    && Renderer2D::getVertexCount() == quads * 4
                    && Renderer2D::getIndexCount() == quads * 6
                    && Renderer2D::getDrawCallCount() == batches.size(),
                    "Renderer statistics disagree with actual submissions");
                require(glGetError() == GL_NO_ERROR, "OpenGL error in renderer boundary check");
            }
        private:
            static void APIENTRY observeUpload(GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) {
                active->uploads.push_back(size);
                active->upload(buffer, size, data, usage);
            }
            static void APIENTRY observeDraw(GLenum mode, GLsizei count, GLenum type, const void* indices) {
                GLint stride = 0;
                glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
                active->draws.push_back({ count, stride });
                active->draw(mode, count, type, indices);
            }
        };
        GLObservation* GLObservation::active = nullptr;

        using Pixel = std::array<unsigned char, 4>;
        constexpr unsigned side = 128;
        constexpr Pixel black = { 0, 0, 0, 255 };
        constexpr Pixel white = { 255, 255, 255, 255 };
        glm::vec4 color(const Pixel& p) {
            return { p[0] / 255.f, p[1] / 255.f, p[2] / 255.f, p[3] / 255.f };
        }
        glm::vec3 position(unsigned cell) {
            return { float(cell % side) + 0.5f, float(cell / side) + 0.5f, 0.f };
        }
        glm::mat4 transform(unsigned cell) {
            return glm::translate(glm::mat4(1.f), position(cell));
        }
        Pixel palette(unsigned i) {
            return { static_cast<unsigned char>(32 + (i * 53) % 224),
                static_cast<unsigned char>(32 + (i * 97) % 224),
                static_cast<unsigned char>(32 + (i * 139) % 224), 255 };
        }
        Ref<Texture2D> texture(Pixel pixel) {
            auto result = Texture2D::create(1, 1);
            result->setData(pixel.data(), 4);
            return result;
        }

        class Target {
        public:
            std::vector<Pixel> expected = std::vector<Pixel>(side * side, black);
            GLuint framebuffer = 0, attachment = 0;
            GLint oldFramebuffer = 0, viewport[4]{};
            GLboolean blend = glIsEnabled(GL_BLEND), depth = glIsEnabled(GL_DEPTH_TEST);
            GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            GLfloat clearColor[4]{};
            Target() {
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
                glGetIntegerv(GL_VIEWPORT, viewport);
                glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
                glCreateFramebuffers(1, &framebuffer);
                glCreateTextures(GL_TEXTURE_2D, 1, &attachment);
                glTextureStorage2D(attachment, 1, GL_RGBA8, side, side);
                glNamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT0, attachment, 0);
                require(glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Renderer regression framebuffer is incomplete");
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glViewport(0, 0, side, side);
                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_SCISSOR_TEST);
            }
            ~Target() {
                glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);
                glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
                if (blend) glEnable(GL_BLEND);
                if (depth) glEnable(GL_DEPTH_TEST);
                if (scissor) glEnable(GL_SCISSOR_TEST);
                glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
                glDeleteFramebuffers(1, &framebuffer);
                glDeleteTextures(1, &attachment);
            }
            void clear() {
                std::fill(expected.begin(), expected.end(), black);
                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            void verify() const {
                std::vector<Pixel> actual(side * side);
                glReadPixels(0, 0, side, side, GL_RGBA, GL_UNSIGNED_BYTE, actual.data());
                for (size_t i = 0; i < actual.size(); ++i) {
                    for (unsigned c = 0; c < 4; ++c) {
                        if (std::abs(int(actual[i][c]) - int(expected[i][c])) > 1) {
                            throw std::runtime_error("Renderer framebuffer mismatch at cell "
                                + std::to_string(i) + ", channel " + std::to_string(c));
                        }
                    }
                }
            }
        };
    }

    void checkRendererBatches() {
        require(GLAD_GL_VERSION_4_5 != 0, "renderer-batches requires OpenGL 4.5");
        GLint units = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
        require(units >= 32, "Renderer shader requires 32 fragment texture units");
        require(glGetError() == GL_NO_ERROR, "OpenGL error before renderer checks");
        Target target;
        std::array<Ref<Texture2D>, 32> textures;
        for (unsigned i = 0; i < textures.size(); ++i) textures[i] = texture(palette(i));
        auto sub = makeRef<SubTexture2D>(textures[0], glm::vec2(0.f), glm::vec2(1.f));
        const auto projection = glm::ortho(0.f, float(side), 0.f, float(side), -1.f, 1.f);
        GLObservation observation;
        auto begin = [&] {
            target.clear();
            observation.reset();
            Renderer2D::resetRendererStat();
            Renderer2D::beginScene(projection);
        };
        auto finish = [&](std::initializer_list<unsigned> batches, unsigned quads) {
            Renderer2D::endScene();
            observation.verify(batches, quads);
            target.verify();
        };

        begin();
        finish({}, 0);
        // A second empty end must not upload, draw, or change statistics.
        Renderer2D::endScene();
        observation.verify({}, 0);

        // Each cell has its own pixel oracle: missing any quad, a bad transform,
        // or a wrong binding fails independently of the renderer counters.
        for (unsigned mode = 0; mode < 10; ++mode) {
            for (unsigned count : { 10000u, 10001u }) {
                begin();
                for (unsigned i = 0; i < count; ++i) {
                    unsigned call = mode >= 8 ? (i + mode - 8) % 8 : mode;
                    const Pixel ink = call == 0 || call == 2 || call == 4 ? palette(i) : palette(0);
                    target.expected[i] = ink;
                    const auto pos = position(i);
                    switch (call) {
                    case 0: Renderer2D::drawColorQuad(transform(i), color(ink)); break;
                    case 1: Renderer2D::drawTextureQuad(transform(i), textures[0]); break;
                    case 2: Renderer2D::drawQuad(pos, { 1, 1 }, color(ink)); break;
                    case 3: Renderer2D::drawQuad(pos, { 1, 1 }, textures[0]); break;
                    case 4: Renderer2D::drawRotatedQuad(pos, { 1, 1 }, glm::radians(90.f), color(ink)); break;
                    case 5: Renderer2D::drawRotatedQuad(pos, { 1, 1 }, glm::radians(90.f), textures[0]); break;
                    case 6: Renderer2D::drawQuad(pos, { 1, 1 }, sub); break;
                    case 7: Renderer2D::drawRotatedQuad(pos, { 1, 1 }, glm::radians(90.f), sub); break;
                    }
                }
                if (count == 10000) finish({ 10000 }, count);
                else finish({ 10000, 1 }, count);
            }
        }

        for (unsigned count : { 31u, 32u }) {
            begin();
            for (unsigned i = 0; i < count; ++i) {
                Renderer2D::drawTextureQuad(transform(i), textures[i]);
                target.expected[i] = palette(i);
            }
            if (count == 31) finish({ 31 }, count);
            else finish({ 31, 1 }, count);
        }
        begin();
        for (unsigned i = 0; i < 31; ++i) {
            Renderer2D::drawTextureQuad(transform(i), textures[i]);
            target.expected[i] = palette(i);
        }
        Renderer2D::drawTextureQuad(transform(31), textures[17]);
        target.expected[31] = palette(17);
        Renderer2D::drawColorQuad(transform(32), color(white));
        target.expected[32] = white; // Slot zero must still be white when slots are full.
        finish({ 33 }, 33);

        // Both capacities full: test a new texture and a previously assigned high
        // slot. A pre-flush lookup would leave the latter's index stale.
        for (unsigned trigger : { 31u, 30u }) {
            begin();
            for (unsigned i = 0; i < 10000; ++i) {
                unsigned slot = i % 31;
                Renderer2D::drawTextureQuad(transform(i), textures[slot]);
                target.expected[i] = palette(slot);
            }
            Renderer2D::drawTextureQuad(transform(10000), textures[trigger]);
            target.expected[10000] = palette(trigger);
            finish({ 10000, 1 }, 10001);
        }

        // Order across a quad flush, using opaque overlapping calls and an
        // asymmetric rotated/scaled matrix. All filler quads are off screen.
        begin();
        for (unsigned i = 0; i < 9999; ++i)
            Renderer2D::drawColorQuad(glm::translate(glm::mat4(1.f), { -5.f, -5.f, 0.f }), color(white));
        Renderer2D::drawColorQuad(transform(0), color(white));
        Renderer2D::drawTextureQuad(transform(0), textures[2]);
        target.expected[0] = palette(2);
        auto matrix = glm::translate(glm::mat4(1.f), { 4.f, 4.f, 0.f })
            * glm::rotate(glm::mat4(1.f), glm::radians(90.f), { 0.f, 0.f, 1.f })
            * glm::scale(glm::mat4(1.f), { 2.f, 4.f, 1.f });
        Renderer2D::drawColorQuad(matrix, color(palette(4)));
        for (unsigned y = 3; y < 5; ++y)
            for (unsigned x = 2; x < 6; ++x) target.expected[y * side + x] = palette(4);
        finish({ 10000, 2 }, 10002);

        // A texture-only flush must also preserve opaque overlap order and the
        // white slot for a following colored quad in the newly started batch.
        begin();
        for (unsigned i = 0; i < 31; ++i)
            Renderer2D::drawTextureQuad(transform(0), textures[i]);
        Renderer2D::drawTextureQuad(transform(0), textures[31]);
        target.expected[0] = palette(31);
        Renderer2D::drawColorQuad(transform(1), color(white));
        target.expected[1] = white;
        finish({ 31, 2 }, 33);

        // Four borrowed UVs are consumed during the call. A nonuniform texture
        // distinguishes them from default UVs and from a retained caller pointer.
        auto atlas = Texture2D::create(2, 2);
        std::array<Pixel, 4> texels = { palette(1), palette(2), palette(3), palette(4) };
        atlas->setData(texels.data(), 16);
        begin();
        glm::vec2 uv[4] = { { .75f, .25f }, { .75f, .25f }, { .75f, .25f }, { .75f, .25f } };
        Renderer2D::drawTextureQuad(transform(0), atlas, uv, 1.f, { .5f, 1.f, 1.f, 1.f });
        for (auto& coordinate : uv) coordinate = { .25f, .75f };
        target.expected[0] = texels[1];
        target.expected[0][0] = static_cast<unsigned char>(std::lround(texels[1][0] * .5f));
        finish({ 1 }, 1);

        // Consecutive scenes use the new camera, with no previous vertices or slots.
        begin();
        Renderer2D::endScene(); // Close the empty matrix scene before switching camera overloads.
        OrthoCamera camera(projection);
        camera.setViewMatrix(glm::translate(glm::mat4(1.f), { 2.f / side, 0.f, 0.f }));
        Renderer2D::beginScene(camera);
        Renderer2D::drawColorQuad(transform(0), color(white));
        target.expected[1] = white;
        finish({ 1 }, 1);
        begin();
        finish({}, 0);
    }
}
