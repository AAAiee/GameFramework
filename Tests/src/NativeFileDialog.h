#pragma once

#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>
#include <UIAutomation.h>
#include <wrl/client.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace Rubber::NativeFileDialog {

    enum class Action { Submit, Cancel, Decline, Confirm, Reject, Capacity };

    namespace Detail {
        inline void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        struct Windows {
            std::vector<HWND> dialogs;
            static BOOL CALLBACK collect(HWND window, LPARAM context) {
                DWORD process = 0;
                GetWindowThreadProcessId(window, &process);
                if (process != GetCurrentProcessId()) return TRUE;
                wchar_t name[32]{};
                GetClassNameW(window, name, 32);
                if (IsWindowVisible(window) && std::wstring_view(name) == L"#32770")
                    reinterpret_cast<Windows*>(context)->dialogs.push_back(window);
                return TRUE;
            }
        };

        inline HWND filenameEdit(HWND dialog) {
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
            if (!edit) {
                // Current Windows dialogs nest the filename Edit (1001) below
                // DirectUI/ComboBox hosts. The address bar has a different ID.
                EnumChildWindows(dialog, [](HWND child, LPARAM context) -> BOOL {
                    wchar_t name[32]{};
                    GetClassNameW(child, name, 32);
                    if (GetDlgCtrlID(child) == 1001 && std::wstring_view(name) == L"Edit") {
                        *reinterpret_cast<HWND*>(context) = child;
                        return FALSE;
                    }
                    return TRUE;
                }, reinterpret_cast<LPARAM>(&edit));
            }
            return edit;
        }

        inline void command(HWND dialog, int id) {
            require(PostMessageW(dialog, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                reinterpret_cast<LPARAM>(GetDlgItem(dialog, id))) != FALSE,
                "Could not send native dialog command");
        }

        using AutomationElement = Microsoft::WRL::ComPtr<IUIAutomationElement>;

        inline std::vector<AutomationElement> accessibleButtons(IUIAutomation* automation, HWND window) {
            AutomationElement root;
            require(SUCCEEDED(automation->ElementFromHandle(window, &root)), "Cannot inspect native prompt");
            Microsoft::WRL::ComPtr<IUIAutomationCondition> condition;
            VARIANT type{};
            type.vt = VT_I4;
            type.lVal = UIA_ButtonControlTypeId;
            require(SUCCEEDED(automation->CreatePropertyCondition(UIA_ControlTypePropertyId, type, &condition)),
                "Cannot query native prompt buttons");
            Microsoft::WRL::ComPtr<IUIAutomationElementArray> elements;
            require(SUCCEEDED(root->FindAll(TreeScope_Descendants, condition.Get(), &elements)),
                "Cannot enumerate native prompt buttons");
            int length = 0;
            elements->get_Length(&length);
            std::vector<AutomationElement> buttons;
            Microsoft::WRL::ComPtr<IUIAutomationTreeWalker> walker;
            require(SUCCEEDED(automation->get_ControlViewWalker(&walker)), "Cannot inspect prompt hierarchy");
            for (int i = 0; i < length; ++i) {
                AutomationElement button;
                elements->GetElement(i, &button);
                BOOL enabled = FALSE;
                button->get_CurrentIsEnabled(&enabled);
                AutomationElement parent;
                walker->GetParentElement(button.Get(), &parent);
                CONTROLTYPEID parentType = 0;
                if (parent) parent->get_CurrentControlType(&parentType);
                // A private desktop is deliberately not the input desktop;
                // providers may label its usable controls as offscreen.
                if (enabled && parentType != UIA_TitleBarControlTypeId) buttons.push_back(button);
            }
            return buttons;
        }

        inline void invoke(IUIAutomationElement* button) {
            Microsoft::WRL::ComPtr<IUIAutomationInvokePattern> pattern;
            require(SUCCEEDED(button->GetCurrentPatternAs(UIA_InvokePatternId, IID_PPV_ARGS(&pattern))),
                "Native prompt button has no invoke action");
            require(SUCCEEDED(pattern->Invoke()), "Cannot invoke native prompt button");
        }
    }

    /** Answers one native file dialog on a duplicate of the UI thread's desktop.
     * Start it before the operation that opens the dialog, then finish() after the
     * owning frame or call returns. File selection, validation and overwrite
     * decisions still execute in the production API.
     */
    class FileDialogSession {
    public:
        FileDialogSession(const char* operation, Action action, std::filesystem::path path)
            : m_Action(action), m_Path(std::move(path)) {
            std::cout << "DIALOG: " << operation << ", action=" << static_cast<int>(action)
                << ", path-code-units=" << m_Path.wstring().size() << std::endl;
            const HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
            m_Thread = std::thread([this, desktop] { watch(desktop); });
        }

        ~FileDialogSession() { finish(); }

        FileDialogSession(const FileDialogSession&) = delete;
        FileDialogSession& operator=(const FileDialogSession&) = delete;

        void finish() {
            m_Finished.store(true);
            if (m_Thread.joinable()) m_Thread.join();
        }

        void rethrow() const {
            if (m_Failure) std::rethrow_exception(m_Failure);
        }

        bool sawDialog() const { return m_Saw.load(); }
        bool submitted() const { return m_Submitted.load(); }
        bool sawPrompt() const { return m_SawPrompt.load(); }
        bool canceled() const { return m_Canceled.load(); }
        bool rejected() const { return m_Rejected.load(); }
        std::size_t visibleDialogs() const { return m_VisibleDialogs.load(); }

    private:
        void watch(HDESK desktop) {
            const bool attached = SetThreadDesktop(desktop) != FALSE;
            const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            Microsoft::WRL::ComPtr<IUIAutomation> automation;
            CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&automation));
            HWND main = nullptr;
            bool promptAnswered = false;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
            try {
                Detail::require(attached && SUCCEEDED(apartment) && automation,
                    "Cannot initialize native dialog automation on the private desktop");
                while (!m_Finished.load()) {
                    Detail::require(std::chrono::steady_clock::now() < deadline,
                        "Native file dialog exceeded 15 second timeout");
                    Detail::Windows windows;
                    EnumDesktopWindows(desktop, Detail::Windows::collect, reinterpret_cast<LPARAM>(&windows));
                    const std::size_t visible = windows.dialogs.size();
                    m_VisibleDialogs.store((std::max)(m_VisibleDialogs.load(), visible));
                    if (!main) {
                        for (HWND window : windows.dialogs) {
                            if (Detail::filenameEdit(window)) { main = window; break; }
                        }
                    }
                    if (main && IsWindowEnabled(main) && !m_Submitted.load() && !m_Canceled.load()) {
                        m_Saw.store(true);
                        if (m_Action == Action::Cancel) {
                            Detail::command(main, IDCANCEL);
                            m_Canceled.store(true);
                        } else {
                            const auto input = m_Path.wstring();
                            DWORD_PTR result = 0;
                            Detail::require(SendMessageTimeoutW(Detail::filenameEdit(main), WM_SETTEXT, 0,
                                reinterpret_cast<LPARAM>(input.c_str()), SMTO_ABORTIFHUNG,
                                2000, &result) != 0 && result != 0,
                                "Could not enter filename in native dialog");
                            std::vector<wchar_t> actual(input.size() + 2);
                            Detail::require(SendMessageTimeoutW(Detail::filenameEdit(main), WM_GETTEXT,
                                actual.size(), reinterpret_cast<LPARAM>(actual.data()),
                                SMTO_ABORTIFHUNG, 2000, &result) != 0,
                                "Could not read native filename field");
                            Detail::require(input == actual.data(), "Native dialog truncated test input before submission");
                            Detail::command(main, IDOK);
                            m_Submitted.store(true);
                        }
                    }
                    for (HWND window : windows.dialogs) {
                        if (!m_Submitted.load() || window == main || !IsWindowEnabled(window)) continue;
                        if (GetDlgItem(window, IDYES) && GetDlgItem(window, IDNO)) {
                            Detail::require(m_Action == Action::Decline || m_Action == Action::Confirm,
                                "Unexpected overwrite confirmation");
                            if (!promptAnswered) {
                                m_SawPrompt.store(true);
                                Detail::command(window, m_Action == Action::Decline ? IDNO : IDYES);
                                promptAnswered = true;
                            }
                        } else if (GetDlgItem(window, IDOK)) {
                            Detail::require(m_Action == Action::Reject || m_Action == Action::Capacity,
                                "Unexpected native dialog validation error");
                            Detail::command(window, IDOK);
                            m_Rejected.store(true);
                        } else if (automation) {
                            const auto buttons = Detail::accessibleButtons(automation.Get(), window);
                            if ((m_Action == Action::Decline || m_Action == Action::Confirm)
                                && buttons.size() == 2 && !promptAnswered) {
                                // Shell DirectUI exposes ordered Yes/No buttons without
                                // HWND IDs. The file assertions verify each choice.
                                Detail::invoke(buttons[m_Action == Action::Confirm ? 0 : 1].Get());
                                m_SawPrompt.store(true);
                                promptAnswered = true;
                            } else if ((m_Action == Action::Reject || m_Action == Action::Capacity)
                                && buttons.size() == 1 && !m_Rejected.load()) {
                                Detail::invoke(buttons.front().Get());
                                m_Rejected.store(true);
                            }
                        }
                    }
                    if (main && IsWindowEnabled(main) && !m_Canceled.load()
                        && ((m_Action == Action::Decline && promptAnswered)
                        || ((m_Action == Action::Reject || m_Action == Action::Capacity) && m_Rejected.load()))) {
                        Detail::command(main, IDCANCEL);
                        m_Canceled.store(true);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            } catch (...) {
                m_Failure = std::current_exception();
                // Unwind the modal call on failure. The PowerShell process timeout
                // is a final bound if Windows cannot process these close requests.
                while (!m_Finished.load()) {
                    Detail::Windows windows;
                    EnumDesktopWindows(desktop, Detail::Windows::collect, reinterpret_cast<LPARAM>(&windows));
                    for (HWND window : windows.dialogs) PostMessageW(window, WM_CLOSE, 0, 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            automation.Reset();
            if (SUCCEEDED(apartment)) CoUninitialize();
        }

        Action m_Action;
        std::filesystem::path m_Path;
        std::atomic<bool> m_Finished{false};
        std::thread m_Thread;
        std::atomic<bool> m_Saw{false};
        std::atomic<bool> m_Submitted{false};
        std::atomic<bool> m_SawPrompt{false};
        std::atomic<bool> m_Canceled{false};
        std::atomic<bool> m_Rejected{false};
        std::atomic<std::size_t> m_VisibleDialogs{0};
        std::exception_ptr m_Failure;
    };

    /** Runs a blocking production call whose native dialog needs a chosen answer. */
    template<class Call>
    void dialog(const char* operation, Action action, const std::filesystem::path& path, Call call) {
        FileDialogSession session(operation, action, path);
        std::exception_ptr callFailure;
        DWORD nativeError = 0;
        try {
            call();
            // Read on the calling UI thread before any subsequent dialog API.
            nativeError = CommDlgExtendedError();
        } catch (...) { callFailure = std::current_exception(); }
        session.finish();
        if (!session.sawDialog() || nativeError != 0 || action == Action::Capacity) {
            std::cerr << "DIALOG RESULT: " << operation
                << ", action=" << static_cast<int>(action)
                << ", CommDlgExtendedError=0x" << std::hex << nativeError << std::dec
                << ", visible-dialogs=" << session.visibleDialogs()
                << ", rejected=" << session.rejected() << ", canceled=" << session.canceled()
                << ", submitted=" << session.submitted() << '\n';
        }
        session.rethrow();
        if (callFailure) std::rethrow_exception(callFailure);
        Detail::require(session.sawDialog(), "Production call did not show a native filename dialog (see DIALOG RESULT)");
        if (action != Action::Cancel)
            Detail::require(session.submitted(), "Filename was not submitted");
        if (action == Action::Decline || action == Action::Confirm)
            Detail::require(session.sawPrompt(), "Existing file did not require overwrite confirmation");
        if (action == Action::Decline || action == Action::Cancel)
            Detail::require(session.canceled(), "Native dialog did not process cancellation");
        if (action == Action::Capacity) {
            // A rejection is evidence only after the oversized filename was
            // actually submitted. Initialization failures cannot test capacity.
            Detail::require(nativeError == FNERR_BUFFERTOOSMALL
                || (nativeError == 0 && session.rejected() && session.canceled()),
                "Capacity check did not observe a native buffer/filename rejection");
        } else {
            Detail::require(nativeError == 0, "Native dialog API failed (see DIALOG RESULT)");
        }
    }

}
