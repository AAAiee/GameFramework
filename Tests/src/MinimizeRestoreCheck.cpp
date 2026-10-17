#include "pch.h"
#include "MinimizeRestoreCheck.h"
#include "EditorLayer.h"
#include "Rubber/Core/GameLoopTimer.h"
#include "Rubber/Event/AppEvent.h"
#include "Rubber/imGui/ImGuiLayer.h"
#include "Rubber/Window/Window.h"
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }
        using Clock = std::chrono::steady_clock;
    }

    // Drives Application's production frame directly, without reopening a closed
    // run loop or changing attachment semantics for late-pushed layers.
    class MinimizeRestoreFixture final : public EditorLayer {
    public:
        void onUpdate(float delta) override {
            require(delta == 1.0f / 120.0f, "Fixed update delta changed");
            ++updates;
            EditorLayer::onUpdate(delta);
        }
        void onImGuiRender() override {
            ++renders;
            EditorLayer::onImGuiRender();
        }
        void check(Application& app) {
            const double step = Application::FIXED_TIME_STAMP;
            double lag = 0.0;
            const auto initial = updates;
            app.updateFixedLayers(step * 0.5, false, lag);
            require(updates == initial, "Half step incorrectly updated layers");
            app.updateFixedLayers(step * 2.75, false, lag);
            require(updates == initial + 3 && std::abs(lag - step * 0.25) < 1e-12,
                "Active time did not produce three updates and retain its remainder");
            for (const auto size : {glm::uvec2(0, 600), glm::uvec2(800, 0)}) {
                app.onWindowResize(WindowResizeEvent(size.x, size.y));
                require(app.m_IsWindowMinimized, "Zero extent did not minimize Application");
                const auto before = updates;
                lag = step * 0.75;
                app.updateFixedLayers(5.0, false, lag);
                app.updateFixedLayers(5.0, true, lag);
                require(updates == before && lag == 0.0, "Minimized work or lag was retained");
                app.onWindowResize(WindowResizeEvent(800, 600));
                require(!app.m_IsWindowMinimized, "Positive resize did not restore Application");
                app.updateFixedLayers(5.0, true, lag);
                require(updates == before && lag == 0.0, "Restore replayed the previous minimized frame");
                app.updateFixedLayers(step, false, lag);
                require(updates == before + 1 && lag == 0.0, "Active updates did not resume immediately");
            }

            auto* window = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
            glfwShowWindow(window);
            glfwRestoreWindow(window);
            // Exercise ordinary editor frames before the real native transition.
            for (int i = 0; i < 6; ++i) app.runFrame(lag);
            const auto beforeActive = updates;
            for (int i = 0; i < 6; ++i) app.runFrame(lag);
            require(updates > beforeActive, "Real active editor frames did not update");

            glfwIconifyWindow(window);
            auto deadline = Clock::now() + std::chrono::seconds(5);
            while (!app.m_IsWindowMinimized || !glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
                require(Clock::now() < deadline, "Native minimize/resize was not delivered");
                app.runFrame(lag);
            }
            const auto minimizedAt = Clock::now();
            const auto stoppedUpdates = updates;
            const auto firstRender = renders;
            while (Clock::now() - minimizedAt < std::chrono::seconds(3)) {
                app.runFrame(lag);
                require(app.m_IsWindowMinimized && glfwGetWindowAttrib(window, GLFW_ICONIFIED),
                    "Window restored unexpectedly during minimized observation");
                require(updates == stoppedUpdates && lag == 0.0,
                    "Real minimized frames accumulated work or performed fixed updates");
            }
            const double minimizedSeconds = std::chrono::duration<double>(Clock::now() - minimizedAt).count();
            require(renders > firstRender + 2, "ImGui stopped rendering while minimized");
            std::cout << "OBSERVED: minimized " << minimizedSeconds << " seconds; fixed updates=0; ImGui frames="
                << renders - firstRender << '\n';

            glfwRestoreWindow(window);
            deadline = Clock::now() + std::chrono::seconds(5);
            while (app.m_IsWindowMinimized) {
                require(Clock::now() < deadline, "Native restore/positive resize was not delivered");
                app.runFrame(lag);
                require(updates == stoppedUpdates && lag == 0.0,
                    "Restore frame replayed minimized work");
            }
            require(!glfwGetWindowAttrib(window, GLFW_ICONIFIED)
                && Application::getWindow().getWidth() > 0 && Application::getWindow().getHeight() > 0,
                "Native window did not restore to positive dimensions");
            const auto restoredAt = Clock::now();
            double activeBudget = 0.0;
            do {
                const auto before = updates;
                const double delta = Application::getTimer()->getDeltaTime();
                require(std::isfinite(delta) && delta >= 0.0, "Timer returned invalid seconds");
                activeBudget += delta;
                app.runFrame(lag);
                require(updates - before <= static_cast<unsigned>(std::ceil(delta / step)) + 1,
                    "Restored frame replayed a minimized backlog");
            } while (Clock::now() - restoredAt < std::chrono::milliseconds(250));
            require(updates > stoppedUpdates && updates - stoppedUpdates <= activeBudget / step + 1,
                "Restored editor did not follow ordinary active-frame time");
            std::cout << "OBSERVED: restored frame updates=0; following active seconds=" << activeBudget
                << "; fixed updates=" << updates - stoppedUpdates << "; delta=1/120 seconds\n";
        }

        static void run(Application& app) {
            const bool ownsImGui = ImGui::GetCurrentContext() == nullptr;
            if (ownsImGui) app.m_ImGuiLayer->onAttach();
            require(ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData
                && ImGui::GetIO().BackendPlatformUserData, "Cannot initialize real editor ImGui backends");
            auto* fixture = new MinimizeRestoreFixture();
            app.pushLayer(fixture);
            try {
                fixture->onAttach();
                // onAttach adds fonts even when the suite already has a warm backend.
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                require(ImGui_ImplOpenGL3_CreateFontsTexture(), "Cannot initialize editor font texture");
                fixture->check(app);
            } catch (...) {
                app.m_LayerStack.popLayer(fixture);
                fixture->onDetach();
                delete fixture;
                if (ownsImGui) app.m_ImGuiLayer->onDetach();
                throw;
            }
            app.m_LayerStack.popLayer(fixture);
            fixture->onDetach();
            delete fixture;
            if (ownsImGui) app.m_ImGuiLayer->onDetach();
        }
    private:
        unsigned updates = 0;
        unsigned renders = 0;
    };

    void checkMinimizeRestore(Application& application) {
        MinimizeRestoreFixture::run(application);
    }
}
