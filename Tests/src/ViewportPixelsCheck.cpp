#include "pch.h"
#include "ViewportPixelsCheck.h"
#include "EditorLayer.h"
#include "Rubber/imGui/ImGuiLayer.h"
#include <glad/glad.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool value, const char* message) {
            if (!value) throw std::runtime_error(message);
        }
        void drain() {
            Application::getEventManager()->dispatchAllEvents();
            Application::getEventManager()->dispatchAllEvents();
        }
        class ImGuiScope {
        public:
            ImGuiScope() : owns(ImGui::GetCurrentContext() == nullptr) {
                if (owns) layer.onAttach();
                require(ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData
                    && ImGui::GetIO().BackendPlatformUserData, "Viewport fixture requires real ImGui backends");
            }
            ~ImGuiScope() { if (owns) layer.onDetach(); }
            ImGuiLayer layer;
            bool owns;
        };
    }

    class ViewportPixelsFixture final : public EditorLayer {
    public:
        void run(ImGuiLayer& imgui) {
            // Exercise the warm backend even when this selector runs alone.
            // NewFrame creates device objects only while ShaderHandle is absent.
            ImGui_ImplOpenGL3_NewFrame();
            onAttach();
            m_FrameBuffer->resize(811, 607);
            auto em = Application::getEventManager();
            try {
                auto entity = m_ActiveScene->createEntity("viewport pixels");
                entity.addComponent<CameraComponent>();
                drain();
                unsigned events = 0;
                glm::uvec2 delivered{};
                em->subscribe<ViewPortResizeEvent>("F06 observer", [&](const ViewPortResizeEvent& e) {
                    ++events;
                    delivered = { e.getWidth(), e.getHeight() };
                    require(delivered.x > 0 && delivered.y > 0, "Unusable viewport event published");
                    return false;
                });
                const auto cameraRatio = [&] {
                    const auto& p = entity.getComponent<CameraComponent>().camera.getProjection();
                    for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r)
                        require(std::isfinite(p[c][r]), "Nonfinite camera projection");
                    return p[1][1] / p[0][0];
                };
                const auto update = [&](glm::vec2 size) {
                    // Inject the sampled panel extent at the real editor's layout/update boundary.
                    m_ViewPortDimension = size;
                    onUpdate(1.0f / 120.0f);
                };
                update({800.5f, 600.5f});
                drain();
                require(events == 1 && delivered == glm::uvec2(800, 600), "Fractional resize event mismatch");
                require(std::abs(cameraRatio() - 800.0f / 600.0f) < 1e-5f, "Camera did not receive quantized pixels");
                const auto framebuffer = m_FrameBuffer->getRendererId();
                const auto color = m_FrameBuffer->getColorAttachmentID();
                const auto depth = m_FrameBuffer->getDepthAttachmentID();
                // Texture state detects recreation even if a driver reuses an attachment name.
                glTextureParameteri(color, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                const auto stable = [&] {
                    require(m_FrameBuffer->getRendererId() == framebuffer
                        && m_FrameBuffer->getColorAttachmentID() == color
                        && m_FrameBuffer->getDepthAttachmentID() == depth, "Stable panel recreated attachments");
                    GLint wrap = 0;
                    glGetTextureParameteriv(color, GL_TEXTURE_WRAP_S, &wrap);
                    require(wrap == GL_CLAMP_TO_EDGE, "Stable panel replaced texture storage");
                    require(events == 1, "Stable/unusable panel republished resize");
                    require(std::abs(cameraRatio() - 800.0f / 600.0f) < 1e-5f, "Unusable size changed camera");
                };
                for (int i = 0; i < 8; ++i) { update({800.5f, 600.5f}); drain(); stable(); }
                std::cout << "OBSERVED: stable 800.5x600.5 panel retained framebuffer and attachments at 800x600\n";
                for (const auto size : {glm::vec2(0,600), glm::vec2(800,0), glm::vec2(-1,600),
                    glm::vec2(-2,-3), glm::vec2(0.5f,600), glm::vec2(800,0.5f),
                    glm::vec2(std::numeric_limits<float>::quiet_NaN(),600),
                    glm::vec2(std::numeric_limits<float>::infinity(),600)}) {
                    update(size); drain(); stable();
                }
                std::cout << "OBSERVED: zero, negative and subpixel-to-zero panels preserved usable framebuffer and camera\n";
                update({801.75f,603.25f});
                require(m_ActiveScene->m_ViewPortDimension == glm::vec2(801,603),
                    "Scene did not store the same integer viewport pixels");
                const auto& spec = m_FrameBuffer->getSpecification();
                require(spec.m_Width == 801 && spec.m_Height == 603, "Changed pixel framebuffer mismatch");
                GLint textureWidth = 0, textureHeight = 0;
                glGetTextureLevelParameteriv(m_FrameBuffer->getColorAttachmentID(), 0, GL_TEXTURE_WIDTH, &textureWidth);
                glGetTextureLevelParameteriv(m_FrameBuffer->getColorAttachmentID(), 0, GL_TEXTURE_HEIGHT, &textureHeight);
                require(textureWidth == 801 && textureHeight == 603
                    && glCheckNamedFramebufferStatus(m_FrameBuffer->getRendererId(), GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Resized GL attachments do not match usable pixels");
                drain();
                require(events == 2 && delivered == glm::uvec2(801,603), "Changed pixel event mismatch");
                require(std::abs(cameraRatio() - 801.0f / 603.0f) < 1e-5f, "Changed pixel camera mismatch");
                // A later camera must receive the same scene-stored integer pixels.
                auto later = m_ActiveScene->createEntity("later camera");
                auto& camera = later.addComponent<CameraComponent>().camera;
                drain();
                const auto& p = camera.getProjection();
                require(delivered == glm::uvec2(801,603)
                    && std::abs(p[1][1] / p[0][0] - 801.0f / 603.0f) < 1e-5f,
                    "Scene retained fractional dimensions for camera hooks");
                GLint binding = -1, viewport[4]{};
                glGetIntegerv(GL_FRAMEBUFFER_BINDING, &binding);
                glGetIntegerv(GL_VIEWPORT, viewport);
                require(binding == 0 && viewport[2] == 801 && viewport[3] == 603,
                    "Editor framebuffer binding/viewport behavior changed");
                // onAttach adds real editor fonts, including when reusing the full-suite atlas.
                // This pinned backend's NewFrame does not refresh fonts once its
                // shader exists. Rebuild explicitly before ImGui::NewFrame.
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                require(ImGui_ImplOpenGL3_CreateFontsTexture(), "Editor font texture rebuild failed");
                const auto* atlas = ImGui::GetIO().Fonts;
                require(atlas->IsBuilt() && atlas->TexID != nullptr,
                    "Editor font atlas is not ready for rendering");
                GLint fontWidth = 0, fontHeight = 0;
                const auto fontTexture = static_cast<GLuint>(reinterpret_cast<uintptr_t>(atlas->TexID));
                glGetTextureLevelParameteriv(fontTexture, 0, GL_TEXTURE_WIDTH, &fontWidth);
                glGetTextureLevelParameteriv(fontTexture, 0, GL_TEXTURE_HEIGHT, &fontHeight);
                require(fontWidth == atlas->TexWidth && fontHeight == atlas->TexHeight
                    && fontWidth > 0 && fontHeight > 0, "Editor font texture does not match rebuilt atlas");
                auto& style = ImGui::GetStyle();
                const float previous = style.WindowMinSize.x;
                const auto flags = ImGui::GetIO().ConfigFlags;
                // The real backend enables docking before its first frame. The
                // pinned ImGui rejects enabling it for the first time on frame
                // two, so cover both paths starting from that production state.
                for (bool docking : {true, false, true}) {
                    if (docking) ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
                    else ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
                    style.WindowMinSize.x = 137.25f;
                    imgui.begin();
                    onImGuiRender();
                    const bool restored = style.WindowMinSize.x == 137.25f;
                    imgui.end();
                    require(restored, "Dockspace did not restore exact WindowMinSize.x");
                }
                style.WindowMinSize.x = previous;
                ImGui::GetIO().ConfigFlags = flags;
                std::cout << "OBSERVED: changed pixels reached framebuffer, events and cameras; dockspace restored style\n";
            } catch (...) {
                em->unsubscribe<ViewPortResizeEvent>("F06 observer");
                onDetach();
                throw;
            }
            em->unsubscribe<ViewPortResizeEvent>("F06 observer");
            onDetach();
        }
    };

    void checkViewportPixels() {
        {
            ImGuiScope imgui;
            {
                ViewportPixelsFixture fixture;
                fixture.run(imgui.layer);
            }
            checkViewportSceneInitialization();
        }
    }
}
