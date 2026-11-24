#include "pch.h"
#include "EditorIssue15Fixture.h"
#include "EditorDriver.h"
#include "NativeFileDialog.h"
#include "ViewportPixelsCheck.h"
#include <cmath>
#include "EditorLayer.h"
#include "Rubber/Input/InputOverlay.h"
#include "Rubber/Window/Window.h"
#include "Rubber/imGui/ImGuiLayer.h"
#include "Rubber/imGui/ItemProbe.h"
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <exception>
#include <stdexcept>
#include <iterator>
#include <windows.h>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        using NativeFileDialog::Action;
        using NativeFileDialog::dialog;

        /** Standalone selection precedes Application::run(), which normally attaches
         * ImGui. Reuse the application's context in the full suite; otherwise own
         * a production adapter until after the editor fixture has been destroyed.
         */
        class DialogImGuiScope {
        public:
            DialogImGuiScope() : m_OwnsContext(ImGui::GetCurrentContext() == nullptr) {
                if (m_OwnsContext) m_Layer.onAttach();
            }

            ~DialogImGuiScope() {
                if (m_OwnsContext) m_Layer.onDetach();
            }

            DialogImGuiScope(const DialogImGuiScope&) = delete;
            DialogImGuiScope& operator=(const DialogImGuiScope&) = delete;

        private:
            ImGuiLayer m_Layer;
            bool m_OwnsContext;
        };

        constexpr auto filter = L"Rubber Scene (*.rubber)\0*.rubber\0\0";

        std::string bytes(const std::filesystem::path& path) {
            std::ifstream input(path, std::ios::binary);
            require(input.is_open(), "Expected scene destination does not exist");
            return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
        }

        void drainEvents() {
            Application::getEventManager()->dispatchAllEvents();
            Application::getEventManager()->dispatchAllEvents();
        }
    }

    /** Uses actual EditorLayer initialization, File menu clicks and native dialogs.
     * No serializer or dialog replacement and no synthetic success path is used.
     */
    class EditorIssue15Fixture final : public EditorLayer {
    public:
        void run() {
            InputOverlay::clear();
            onAttach();
            ImGui_ImplOpenGL3_DestroyFontsTexture();
            require(ImGui_ImplOpenGL3_CreateFontsTexture(), "Dialog fixture font texture rebuild failed");
            ItemProbe::enable();
            try {
                frame();
                frame();
                check();
            } catch (...) {
                ItemProbe::disable();
                InputOverlay::clear();
                onDetach();
                throw;
            }
            ItemProbe::disable();
            InputOverlay::clear();
            onDetach();
            drainEvents();
        }

        void runViewportInitialization() {
            onAttach();
            try {
                const auto root = std::filesystem::current_path()
                    / ("issue-17-work-" + std::to_string(GetCurrentProcessId()));
                require(!std::filesystem::exists(root), "Viewport working directory already exists");
                std::filesystem::create_directory(root);
                const auto file = root / "viewport.rubber";
                auto initial = m_ActiveScene->createEntity("F06 open camera");
                initial.addComponent<CameraComponent>();
                SceneSerializer(m_ActiveScene).serialize(file);
                drainEvents();
                unsigned events = 0;
                glm::uvec2 delivered{};
                const auto em = Application::getEventManager();
                em->subscribe<ViewPortResizeEvent>("F06 initialization observer", [&](const ViewPortResizeEvent& e) {
                    ++events;
                    delivered = {e.getWidth(), e.getHeight()};
                    require(delivered.x > 0 && delivered.y > 0, "New/Open published unusable pixels");
                    return false;
                });
                // Render target is deliberately different from Scene's default 1600x900.
                m_FrameBuffer->resize(811, 607);
                for (const auto size : {glm::vec2(800.5f,600.5f), glm::vec2(0,0),
                    glm::vec2(-2,600), glm::vec2(-2,-3), glm::vec2(0.5f,600), glm::vec2(800,0.5f)}) {
                    m_ViewPortDimension = size;
                    const glm::uvec2 expected = size.x >= 1 && size.y >= 1
                        ? glm::uvec2(800,600) : glm::uvec2(811,607);
                    const auto verify = [&] {
                        drainEvents();
                        require(events == 1 && delivered == expected, "New/Open did not retain usable pixel dimensions");
                        const auto& p = entity().getComponent<CameraComponent>().camera.getProjection();
                        for (int c=0; c<4; ++c) for (int r=0; r<4; ++r)
                            require(std::isfinite(p[c][r]), "New/Open created invalid projection");
                        require(std::abs(p[1][1] / p[0][0] - float(expected.x) / expected.y) < 1e-5f,
                            "New/Open camera does not match usable pixels");
                    };
                    events = 0;
                    newScene();
                    m_ActiveScene->createEntity("new camera").addComponent<CameraComponent>();
                    verify();
                    events = 0;
                    dialog("F06 Editor Open", Action::Submit, file, [&] { openScene(); });
                    verify();
                }
                em->unsubscribe<ViewPortResizeEvent>("F06 initialization observer");
                std::cout << "OBSERVED: real New/Open used integer pixels or last usable framebuffer for fractional/zero/negative/subpixel sizes\n";
            } catch (...) {
                Application::getEventManager()->unsubscribe<ViewPortResizeEvent>("F06 initialization observer");
                onDetach();
                throw;
            }
            onDetach();
            drainEvents();
        }

    private:
        Entity entity() {
            auto view = m_ActiveScene->getRegistry().view<TagComponent>();
            require(view.size() == 1, "Reopened scene does not contain exactly one entity");
            return { *view.begin(), m_ActiveScene.get() };
        }

        void verify(const std::string& tag) {
            auto e = entity();
            require(e.getComponent<TagComponent>().tag == tag, "Scene tag did not round trip");
            require(e.hasComponent<TransformComponent>() && e.hasComponent<SpriteComponent>()
                && e.hasComponent<CameraComponent>() && e.hasComponent<VisibilityControlComponent>(),
                "Supported scene components did not round trip");
            const auto& transform = e.getComponent<TransformComponent>();
            require(transform.position == glm::vec3(1, 2, 3)
                && transform.rotation == glm::vec3(0.1f, 0.2f, 0.3f)
                && transform.scale == glm::vec3(2, 3, 4), "Transform data changed on reopen");
            require(e.getComponent<SpriteComponent>().color == glm::vec4(0.25f, 0.5f, 0.75f, 1),
                "Sprite color changed on reopen");
            const auto& camera = e.getComponent<CameraComponent>();
            require(camera.isPrimary && camera.isFixedAspectRatio
                && camera.camera.getOrthoSize() == 7.0f, "Camera settings changed on reopen");
            require(!e.getComponent<VisibilityControlComponent>().isVisible,
                "Visibility flag changed on reopen");
        }

        void check() {
            // Work only beneath this executable's output working directory.
            const auto root = std::filesystem::current_path()
                / ("issue-15-work-" + std::to_string(GetCurrentProcessId()));
            require(!std::filesystem::exists(root), "Fixture working directory already exists");
            std::filesystem::create_directory(root);
            m_ViewPortDimension = { 640, 480 };
            m_ActiveScene->updatesViewportSize(m_ViewPortDimension);
            auto e = m_ActiveScene->createEntity("F04 original");
            auto& transform = e.addComponent<TransformComponent>();
            transform.position = { 1, 2, 3 };
            transform.rotation = { 0.1f, 0.2f, 0.3f };
            transform.scale = { 2, 3, 4 };
            e.addComponent<SpriteComponent>().color = { 0.25f, 0.5f, 0.75f, 1 };
            auto& camera = e.addComponent<CameraComponent>();
            camera.isPrimary = true;
            camera.isFixedAspectRatio = true;
            camera.camera.setOrthoSize(7.0f);
            e.addComponent<VisibilityControlComponent>().isVisible = false;
            drainEvents();

            const auto destination = root / "new-scene.rubber";
            require(destination.wstring().size() < 250,
                "Binary output directory is too long for normal-path dialog checks");
            menuSaveAs(Action::Submit, destination);
            const auto original = bytes(destination);
            require(original.find("123123141") != std::string::npos, "Placeholder entity identity missing");
            entity().getComponent<TagComponent>().tag = "unsaved edit";
            menuOpen(destination);
            verify("F04 original");
            drainEvents();
            std::cout << "OBSERVED: new scene saved and reopened with supported component data\n";

            entity().getComponent<TagComponent>().tag = "F04 replacement";
            menuSaveAs(Action::Decline, destination);
            require(bytes(destination) == original, "Declining overwrite changed destination bytes");
            menuSaveAs(Action::Cancel, {});
            require(bytes(destination) == original, "Canceling editor Save As changed destination bytes");
            std::wstring result = L"not empty";
            dialog("fileDialog Save As", Action::Cancel, {}, [&] { result = fileDialog::saveFile(filter); });
            require(result.empty(), "Canceled Save As did not return an empty path");
            dialog("fileDialog Save As", Action::Decline, destination, [&] { result = fileDialog::saveFile(filter); });
            require(result.empty(), "Decline then cancel did not return an empty path");
            require(bytes(destination) == original, "Decline/cancel changed destination");
            menuSaveAs(Action::Confirm, destination);
            require(bytes(destination) != original, "Confirmed replacement did not write new scene");
            menuOpen(destination);
            verify("F04 replacement");
            drainEvents();
            std::cout << "OBSERVED: overwrite prompted; decline/cancel preserved bytes; confirmation replaced scene\n";

            const auto unicode = root / L"\u573a\u666f-\u00e9-\u03a9.rubber";
            dialog("fileDialog Save As", Action::Submit, unicode, [&] { result = fileDialog::saveFile(filter); });
            require(result == unicode.wstring(), "Save As changed the Unicode path");
            // Exercise the editor's entire save/open path too, not just path selection.
            menuSaveAs(Action::Submit, unicode);
            require(bytes(unicode) == bytes(destination), "Unicode scene file differs from saved scene");
            entity().getComponent<TagComponent>().tag = "unsaved Unicode edit";
            menuOpen(unicode);
            verify("F04 replacement");
            drainEvents();
            dialog("fileDialog Open", Action::Submit, unicode, [&] { result = fileDialog::openFile(filter); });
            require(result == unicode.wstring(), "Open changed the Unicode path");
            dialog("fileDialog Open", Action::Cancel, {}, [&] { result = fileDialog::openFile(filter); });
            require(result.empty(), "Canceled Open did not return an empty path");
            const auto missing = root / "missing.rubber";
            dialog("fileDialog Open", Action::Reject, missing, [&] { result = fileDialog::openFile(filter); });
            require(result.empty() && !std::filesystem::exists(missing),
                "Open accepted or created a nonexistent file");
            std::cout << "OBSERVED: Unicode Save As/Open preserved path and scene; Open rejected missing file\n";

            // Existing long filenames may be returned as short 8.3 aliases by
            // Open. Build the boundary entirely from already-short components so
            // Windows cannot satisfy the request by shortening the output path.
            std::vector<wchar_t> shortRoot(32768);
            const DWORD shortLength = GetShortPathNameW(root.c_str(), shortRoot.data(),
                static_cast<DWORD>(shortRoot.size()));
            require(shortLength > 0 && shortLength < shortRoot.size(),
                "Cannot obtain the capacity fixture's short root path");
            std::filesystem::path boundaryRoot(shortRoot.data());
            const auto extended = [](const std::filesystem::path& path) {
                const auto normal = path.wstring();
                return std::filesystem::path(normal.starts_with(L"\\\\")
                    ? L"\\\\?\\UNC\\" + normal.substr(2) : L"\\\\?\\" + normal);
            };
            while (boundaryRoot.wstring().size() + 9 < 260) {
                boundaryRoot /= boundaryRoot.wstring().size() + 9 == 259 ? L"capacit" : L"capacity";
                std::filesystem::create_directory(extended(boundaryRoot));
            }
            const auto filenameLength = 260 - boundaryRoot.wstring().size() - 1;
            require(filenameLength > 0 && filenameLength <= 8, "Invalid short-component capacity fixture");
            const auto oversized = boundaryRoot / std::wstring(filenameLength, L'x');
            require(oversized.wstring().size() == 260, "Capacity boundary fixture has wrong length");
            const auto setupFile = extended(oversized);
            dialog("fileDialog Save As", Action::Capacity, oversized, [&] { result = fileDialog::saveFile(filter); });
            require(result.empty() && !std::filesystem::exists(setupFile),
                "Save As accepted an over-capacity path");
            // Existing oversized file ensures Open tests capacity, not existence.
            std::filesystem::copy_file(destination, setupFile);
            const auto oversizedBytes = bytes(setupFile);
            dialog("fileDialog Open", Action::Capacity, oversized, [&] { result = fileDialog::openFile(filter); });
            require(result.empty() && bytes(setupFile) == oversizedBytes,
                "Open accepted or changed an over-capacity destination");
            require(bytes(destination) == bytes(unicode), "Rejected operations changed a saved destination");
            require(std::filesystem::current_path() == root.parent_path(),
                "Native dialogs changed the working directory");
            std::cout << "OBSERVED: both dialogs rejected over-capacity paths without writing or crashing\n";
        }

        /** Let EditorLayer draw the menu, then answer the same production dialog. */
        void menuSaveAs(Action action, const std::filesystem::path& path) {
            NativeFileDialog::FileDialogSession session("Editor Save As", action, path);
            m_Driver.click("File");
            settle(1);
            m_Driver.click("Save As...");
            settle(3);
            session.finish();
            session.rethrow();
            const DWORD error = CommDlgExtendedError();
            require(session.sawDialog(), "File / Save As... did not open a native dialog");
            if (action == Action::Cancel) {
                require(session.canceled(), "Canceling editor Save As did not reach the dialog");
            } else {
                require(session.submitted(), "File / Save As... did not submit a filename");
                if (action == Action::Decline)
                    require(session.sawPrompt() && session.canceled(),
                        "Declining overwrite did not answer the native prompt");
                if (action == Action::Confirm)
                    require(session.sawPrompt(), "Existing file did not require overwrite confirmation");
            }
            require(error == 0, "File / Save As... dialog API failed");
        }

        void menuOpen(const std::filesystem::path& path) {
            NativeFileDialog::FileDialogSession session("Editor Open", Action::Submit, path);
            m_Driver.click("File");
            settle(1);
            m_Driver.click("Open...");
            settle(3);
            session.finish();
            session.rethrow();
            const DWORD error = CommDlgExtendedError();
            require(session.sawDialog() && session.submitted(), "File / Open... did not submit a filename");
            require(error == 0, "File / Open... dialog API failed");
        }

        void frame() {
            require(++m_Frames < 1200, "Dialog fixture exceeded frame bound");
            m_Driver.tick();
            m_Layer.begin();
            EditorLayer::onImGuiRender();
            m_Layer.end();
        }

        void settle(int extra = 1) {
            unsigned guard = 0;
            while (m_Driver.busy()) {
                require(++guard < 64, "Driver action did not finish");
                frame();
            }
            for (int i = 0; i < extra; ++i) frame();
        }

        ImGuiLayer m_Layer;
        EditorDriver m_Driver;
        unsigned m_Frames = 0;
    };

    void checkViewportSceneInitialization() {
        require(ImGui::GetCurrentContext() != nullptr, "Viewport Open requires a live ImGui context");
        auto* window = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
        HWND owner = glfwGetWin32Window(window);
        require(IsWindow(owner) && GetWindowThreadProcessId(owner, nullptr) == GetCurrentThreadId(),
            "Viewport Open requires the Application UI thread");
        glfwShowWindow(window);
        EditorIssue15Fixture fixture;
        fixture.runViewportInitialization();
    }

    void checkEditorIssue15() {
        auto* window = static_cast<GLFWwindow*>(Application::getWindow().getNativeWindow());
        require(window != nullptr, "Dialog fixture has no Application window");
        HWND owner = glfwGetWin32Window(window);
        require(IsWindow(owner) != FALSE, "Dialog fixture Application HWND is invalid");
        require(GetWindowThreadProcessId(owner, nullptr) == GetCurrentThreadId(),
            "Dialog fixture must run on the Application window UI thread");
        glfwShowWindow(window);
        DialogImGuiScope imgui;
        require(ImGui::GetCurrentContext() != nullptr,
            "Dialog fixture requires an initialized ImGui context before editor attachment");
        require(ImGui::GetIO().BackendPlatformUserData != nullptr
            && ImGui::GetIO().BackendRendererUserData != nullptr,
            "Dialog fixture requires the production ImGui platform and renderer adapters");
        EditorIssue15Fixture fixture;
        fixture.run();
    }
}
