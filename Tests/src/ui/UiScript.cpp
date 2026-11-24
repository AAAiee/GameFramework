#include "pch.h"
#include "UiScript.h"
#include "EditorLayer.h"
#include "Rubber/Core/Application.h"
#include "Rubber/Input/InputOverlay.h"
#include "Rubber/Window/Window.h"
#include "Rubber/imGui/ItemProbe.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <sstream>

namespace Rubber::UiScript {
    namespace {
        class ScriptFixture final : public EditorLayer {
        public:
            explicit ScriptFixture(Script (*entry)(Context&))
                : m_Context{ m_Driver, *this },
                  m_Script(entry(m_Context)) {}

            void onAttach() override {
                EditorLayer::onAttach();
                ItemProbe::enable();
            }

            void onDetach() {
                InputOverlay::clear();
                ItemProbe::disable();
                EditorLayer::onDetach();
            }

            void onImGuiRender() override {
                EditorLayer::onImGuiRender();
                ++m_Frames;
                require(m_Frames < 2400, "Editor UI script timed out");
                if (m_Completed) return;
                m_Driver.tick();
                if (m_Driver.busy()) return;
                if (m_Driver.dialogCancelPending()) {
                    require(m_Driver.sawDialog(), "File menu did not open a native dialog");
                    m_Context.dialogSeen = true;
                    m_Driver.endDialogCancel();
                }
                if (m_Context.closing) {
                    m_Completed = true;
                    return;
                }
                m_Script.resume();
                m_Script.rethrow();
                if (m_Context.closing) return;
                if (m_Script.done()) {
                    m_Completed = true;
                    Application::close();
                }
            }

            bool completed() const { return m_Completed; }

            void release() { onDetach(); }

        private:
            EditorDriver m_Driver;
            Context m_Context;
            Script m_Script;
            unsigned m_Frames = 0;
            bool m_Completed = false;
        };
    }

    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    std::size_t taggedCount(Scene& scene) {
        std::size_t count = 0;
        for (auto _ : scene.getRegistry().view<TagComponent>()) {
            (void)_;
            ++count;
        }
        return count;
    }

    Entity firstTagged(Scene& scene) {
        require(taggedCount(scene) == 1, "Scene has no tagged entity");
        auto view = scene.getRegistry().view<TagComponent>();
        return Entity{*view.begin(), &scene};
    }

    void expect(std::string_view path, int occurrence) {
        if (ItemProbe::find(path, occurrence)) return;
        std::ostringstream message;
        message << "Expected UI path missing: " << path << " occurrence=" << occurrence << "\n";
        ItemProbe::dump(message);
        throw std::runtime_error(message.str());
    }

    void run(Application& application, Script (*entry)(Context&)) {
        auto* fixture = new ScriptFixture(entry);
        application.pushLayer(fixture);
        glfwShowWindow(static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow()));
        application.run();
        require(fixture->completed(), "Editor UI script exited before it finished");
        fixture->release();
    }
}
