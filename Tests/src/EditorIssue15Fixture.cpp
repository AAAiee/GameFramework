#include "pch.h"
#include "EditorIssue15Fixture.h"
#include "EditorLayer.h"
#include "Rubber/Window/Window.h"
#include "Rubber/imGui/ImGuiLayer.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <atomic>
#include <thread>
#include <exception>
#include <stdexcept>
#include <iterator>
#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

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
        enum class Action { Submit, Cancel, Decline, Confirm, Reject, Capacity };

        struct Windows {
            std::vector<HWND> dialogs;
            static BOOL CALLBACK collect(HWND window, LPARAM context) {
                wchar_t name[32]{};
                GetClassNameW(window, name, 32);
                if (IsWindowVisible(window) && std::wstring_view(name) == L"#32770")
                    reinterpret_cast<Windows*>(context)->dialogs.push_back(window);
                return TRUE;
            }
        };

        HWND filenameEdit(HWND dialog) {
            // Legacy common dialogs use edt1; Explorer-style dialogs use cmb13.
            if (HWND edit = GetDlgItem(dialog, edt1)) return edit;
            HWND edit = nullptr;
            if (HWND combo = GetDlgItem(dialog, cmb13)) {
                EnumChildWindows(combo, [](HWND child, LPARAM context) -> BOOL {
                    wchar_t name[32]{};
                    GetClassNameW(child, name, 32);
                    if (std::wstring_view(name) == L"Edit") {
                        *reinterpret_cast<HWND*>(context) = child;
                        return FALSE;
                    }
                    return TRUE;
                }, reinterpret_cast<LPARAM>(&edit));
            }
            return edit;
        }

        void command(HWND dialog, int id) {
            require(PostMessageW(dialog, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                reinterpret_cast<LPARAM>(GetDlgItem(dialog, id))) != FALSE,
                "Could not send native dialog command");
        }

        /** Drives only native dialogs on the calling UI thread. All file selection,
         * validation and overwrite decisions still execute in the production API.
         * Polling discovers controls; elapsed time alone never establishes success.
         */
        template<class Call>
        void dialog(const char* operation, Action action, const std::filesystem::path& path, Call call) {
            std::cout << "DIALOG: " << operation << ", action=" << static_cast<int>(action)
                << ", path-code-units=" << path.wstring().size() << std::endl;
            const DWORD uiThread = GetCurrentThreadId();
            std::atomic<bool> finished = false;
            std::exception_ptr driverFailure;
            bool sawDialog = false, submitted = false, sawPrompt = false;
            bool canceled = false, rejected = false;
            size_t visibleDialogs = 0;
            std::thread driver([&] {
                HWND main = nullptr;
                bool promptAnswered = false;
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
                try {
                    while (!finished.load()) {
                        require(std::chrono::steady_clock::now() < deadline,
                            "Native file dialog exceeded 15 second timeout");
                        Windows windows;
                        EnumThreadWindows(uiThread, Windows::collect, reinterpret_cast<LPARAM>(&windows));
                        visibleDialogs = (std::max)(visibleDialogs, windows.dialogs.size());
                        if (!main) {
                            for (HWND window : windows.dialogs) {
                                if (filenameEdit(window)) { main = window; break; }
                            }
                        }
                        if (main && IsWindowEnabled(main) && !submitted && !canceled) {
                            sawDialog = true;
                            if (action == Action::Cancel) {
                                command(main, IDCANCEL);
                                canceled = true;
                            } else {
                                const auto input = path.wstring();
                                DWORD_PTR result = 0;
                                require(SendMessageTimeoutW(filenameEdit(main), WM_SETTEXT, 0,
                                    reinterpret_cast<LPARAM>(input.c_str()), SMTO_ABORTIFHUNG,
                                    2000, &result) != 0 && result != 0,
                                    "Could not enter filename in native dialog");
                                std::vector<wchar_t> actual(input.size() + 2);
                                require(SendMessageTimeoutW(filenameEdit(main), WM_GETTEXT,
                                    actual.size(), reinterpret_cast<LPARAM>(actual.data()),
                                    SMTO_ABORTIFHUNG, 2000, &result) != 0,
                                    "Could not read native filename field");
                                require(input == actual.data(), "Native dialog truncated test input before submission");
                                command(main, IDOK);
                                submitted = true;
                            }
                        }
                        for (HWND window : windows.dialogs) {
                            if (!submitted || window == main || !IsWindowEnabled(window)) continue;
                            if (GetDlgItem(window, IDYES) && GetDlgItem(window, IDNO)) {
                                require(action == Action::Decline || action == Action::Confirm,
                                    "Unexpected overwrite confirmation");
                                if (!promptAnswered) {
                                    sawPrompt = true;
                                    command(window, action == Action::Decline ? IDNO : IDYES);
                                    promptAnswered = true;
                                }
                            } else if (GetDlgItem(window, IDOK)) {
                                require(action == Action::Reject || action == Action::Capacity,
                                    "Unexpected native dialog validation error");
                                command(window, IDOK);
                                rejected = true;
                            }
                        }
                        if (main && IsWindowEnabled(main) && !canceled
                            && ((action == Action::Decline && promptAnswered)
                            || ((action == Action::Reject || action == Action::Capacity) && rejected))) {
                            command(main, IDCANCEL);
                            canceled = true;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                } catch (...) {
                    driverFailure = std::current_exception();
                    // Unwind the modal call on failure. The PowerShell process timeout
                    // is a final bound if Windows cannot process these close requests.
                    while (!finished.load()) {
                        Windows windows;
                        EnumThreadWindows(uiThread, Windows::collect, reinterpret_cast<LPARAM>(&windows));
                        for (HWND window : windows.dialogs) PostMessageW(window, WM_CLOSE, 0, 0);
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
            });
            std::exception_ptr callFailure;
            DWORD nativeError = 0;
            try {
                call();
                // Read on the calling UI thread before any subsequent dialog API.
                nativeError = CommDlgExtendedError();
            } catch (...) { callFailure = std::current_exception(); }
            finished.store(true);
            driver.join();
            if (!sawDialog || nativeError != 0) {
                std::cerr << "DIALOG RESULT: " << operation
                    << ", action=" << static_cast<int>(action)
                    << ", CommDlgExtendedError=0x" << std::hex << nativeError << std::dec
                    << ", visible-dialogs=" << visibleDialogs
                    << ", submitted=" << submitted << '\n';
            }
            if (driverFailure) std::rethrow_exception(driverFailure);
            if (callFailure) std::rethrow_exception(callFailure);
            require(sawDialog, "Production call did not show a native filename dialog (see DIALOG RESULT)");
            if (action != Action::Cancel)
                require(submitted, "Filename was not submitted");
            if (action == Action::Decline || action == Action::Confirm)
                require(sawPrompt, "Existing file did not require overwrite confirmation");
            if (action == Action::Decline || action == Action::Cancel)
                require(canceled, "Native dialog did not process cancellation");
            if (action == Action::Capacity) {
                // A rejection is evidence only after the oversized filename was
                // actually submitted. Initialization failures cannot test capacity.
                require(nativeError == FNERR_BUFFERTOOSMALL
                    || (nativeError == 0 && rejected && canceled),
                    "Capacity check did not observe a native buffer/filename rejection");
            } else {
                require(nativeError == 0, "Native dialog API failed (see DIALOG RESULT)");
            }
        }

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

    /** Uses actual EditorLayer initialization and Save As/Open entry points.
     * No serializer or dialog replacement and no synthetic success path is used.
     */
    class EditorIssue15Fixture final : public EditorLayer {
    public:
        void run() {
            onAttach();
            try { check(); } catch (...) { onDetach(); throw; }
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
            dialog("Editor Save As", Action::Submit, destination, [&] { saveScene(); });
            const auto original = bytes(destination);
            require(original.find("123123141") != std::string::npos, "Placeholder entity identity missing");
            entity().getComponent<TagComponent>().tag = "unsaved edit";
            dialog("Editor Open", Action::Submit, destination, [&] { openScene(); });
            verify("F04 original");
            drainEvents();
            std::cout << "OBSERVED: new scene saved and reopened with supported component data\n";

            entity().getComponent<TagComponent>().tag = "F04 replacement";
            dialog("Editor Save As", Action::Decline, destination, [&] { saveScene(); });
            require(bytes(destination) == original, "Declining overwrite changed destination bytes");
            dialog("Editor Save As", Action::Cancel, {}, [&] { saveScene(); });
            require(bytes(destination) == original, "Canceling editor Save As changed destination bytes");
            std::wstring result = L"not empty";
            dialog("fileDialog Save As", Action::Cancel, {}, [&] { result = fileDialog::saveFile(filter); });
            require(result.empty(), "Canceled Save As did not return an empty path");
            dialog("fileDialog Save As", Action::Decline, destination, [&] { result = fileDialog::saveFile(filter); });
            require(result.empty(), "Decline then cancel did not return an empty path");
            require(bytes(destination) == original, "Decline/cancel changed destination");
            dialog("Editor Save As", Action::Confirm, destination, [&] { saveScene(); });
            require(bytes(destination) != original, "Confirmed replacement did not write new scene");
            dialog("Editor Open", Action::Submit, destination, [&] { openScene(); });
            verify("F04 replacement");
            drainEvents();
            std::cout << "OBSERVED: overwrite prompted; decline/cancel preserved bytes; confirmation replaced scene\n";

            const auto unicode = root / L"\u573a\u666f-\u00e9-\u03a9.rubber";
            dialog("fileDialog Save As", Action::Submit, unicode, [&] { result = fileDialog::saveFile(filter); });
            require(result == unicode.wstring(), "Save As changed the Unicode path");
            // Exercise the editor's entire save/open path too, not just path selection.
            dialog("Editor Save As", Action::Submit, unicode, [&] { saveScene(); });
            require(bytes(unicode) == bytes(destination), "Unicode scene file differs from saved scene");
            entity().getComponent<TagComponent>().tag = "unsaved Unicode edit";
            dialog("Editor Open", Action::Submit, unicode, [&] { openScene(); });
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

            // Exactly 260 path code units requires 261 buffer elements with null.
            // Keep the filename component below 255 and use an existing directory.
            const std::wstring suffix = L".rubber";
            const auto filenameLength = 260 - root.wstring().size() - 1;
            require(filenameLength > suffix.size() && filenameLength <= 255,
                "Cannot form a valid single-component capacity fixture");
            const auto oversized = root / (std::wstring(filenameLength - suffix.size(), L'x') + suffix);
            require(oversized.wstring().size() == 260, "Capacity boundary fixture has wrong length");
            // Extended spelling only prepares/reads a fixture on hosts without
            // longPathAware; production dialogs receive the ordinary spelling.
            const auto normal = oversized.wstring();
            const auto setupFile = std::filesystem::path(normal.starts_with(L"\\\\")
                ? L"\\\\?\\UNC\\" + normal.substr(2) : L"\\\\?\\" + normal);
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
    };

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
