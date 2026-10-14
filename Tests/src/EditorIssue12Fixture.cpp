#include "pch.h"
#include "EditorIssue12Fixture.h"
#include "EditorLayer.h"
#include "Rubber/Window/Window.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) {
                throw std::runtime_error(message);
            }
        }
    }

    /** Exercises the production EditorLayer using its actual scene, framebuffer and ImGui panels. */
    class EditorIssue12Fixture final : public EditorLayer {
    public:
        void onAttach() override {
            EditorLayer::onAttach();
            require(m_ActiveScene != nullptr && m_FrameBuffer != nullptr,
                "Editor did not initialize its scene and framebuffer");
            m_Entity = m_ActiveScene->createEntity("F01 editor fixture");
            m_Entity.addComponent<TransformComponent>();
            m_Entity.addComponent<SpriteComponent>();
            m_Entity.addComponent<VisibilityControlComponent>();
            m_Entity.addComponent<CameraComponent>().isPrimary = true;
        }

        /** Waits two application frame dispatches after the latest viewport change.
         * Fixed updates can repeat within a frame; they do not advance event delivery.
         */
        void onUpdate(float ts) override {
            if (m_Completed) {
                return;
            }
            EditorLayer::onUpdate(ts);
            if (m_ViewPortDimension != m_LastViewportDimension) {
                m_LastViewportDimension = m_ViewPortDimension;
                m_ViewportChangedFrame = m_Frames;
            }
            if (m_Frames >= 3 && m_Frames - m_ViewportChangedFrame >= 2
                && m_ViewPortDimension.x > 0 && m_ViewPortDimension.y > 0) {
                const auto& spec = m_FrameBuffer->getSpecification();
                require(spec.m_Width == static_cast<uint32_t>(m_ViewPortDimension.x)
                    && spec.m_Height == static_cast<uint32_t>(m_ViewPortDimension.y),
                    "Editor framebuffer did not follow the viewport");
                require(Renderer2D::getQuadCount() == 1, "Editor did not render the scene sprite");
                const auto& projection = m_Entity.getComponent<CameraComponent>().camera.getProjection();
                const float ratio = projection[1][1] / projection[0][0];
                require(std::isfinite(ratio) && std::abs(ratio
                    - static_cast<float>(spec.m_Width) / static_cast<float>(spec.m_Height)) < 0.01f,
                    "Installed WindowSystem did not deliver the camera viewport resize");
                m_Entity.getComponent<TagComponent>().tag = "edited entity";
                require(m_Entity.getComponent<TagComponent>().tag == "edited entity",
                    "Editor scene tag edit failed");
                m_Entity.removeComponent<CameraComponent>();
                m_Entity.removeComponent<SpriteComponent>();
                m_Entity.removeComponent<VisibilityControlComponent>();
                m_Entity.removeComponent<TransformComponent>();
                m_Entity.removeComponent<TagComponent>();
                require(!m_Entity.hasComponent<TagComponent>()
                    && !m_Entity.hasComponent<TransformComponent>()
                    && !m_Entity.hasComponent<SpriteComponent>()
                    && !m_Entity.hasComponent<CameraComponent>()
                    && !m_Entity.hasComponent<VisibilityControlComponent>(),
                    "Editor component removal left registry storage attached");
                const auto handle = static_cast<entt::entity>(m_Entity);
                m_ActiveScene->removeEntity(m_Entity);
                require(!m_ActiveScene->getRegistry().valid(handle), "Editor entity deletion failed");
                Application::getEventManager()->dispatchAllEvents();
                Application::getEventManager()->dispatchAllEvents();
                m_Completed = true;
                Application::close();
            }
        }

        void onImGuiRender() override {
            EditorLayer::onImGuiRender();
            ++m_Frames;
            require(m_Frames < 600, "Editor fixture timed out waiting for updates and viewport");
        }

        bool completed() const { return m_Completed; }

    private:
        Entity m_Entity;
        glm::vec2 m_LastViewportDimension = { 0.0f, 0.0f };
        unsigned m_ViewportChangedFrame = 0;
        unsigned m_Frames = 0;
        bool m_Completed = false;
    };

    void checkEditorIssue12(Application& application) {
        auto* fixture = new EditorIssue12Fixture();
        application.pushLayer(fixture);
        glfwShowWindow(static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow()));
        application.run();
        require(fixture->completed(), "Editor loop exited before behavioral checks completed");
    }
}
