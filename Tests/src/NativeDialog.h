#pragma once

#include <windows.h>
#include <atomic>
#include <chrono>
#include <string_view>
#include <thread>

namespace Rubber::NativeDialog {

    class CancelDialogSession {
    public:
        CancelDialogSession() {
            const HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
            m_Thread = std::thread([this, desktop] {
                if (!SetThreadDesktop(desktop)) return;
                while (!m_Finished.load()) {
                    EnumDesktopWindows(desktop, collect, reinterpret_cast<LPARAM>(this));
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }

        ~CancelDialogSession() {
            m_Finished.store(true);
            if (m_Thread.joinable()) m_Thread.join();
        }

        bool sawDialog() const { return m_Saw.load(); }

    private:
        static BOOL CALLBACK collect(HWND window, LPARAM context) {
            auto* self = reinterpret_cast<CancelDialogSession*>(context);
            DWORD process = 0;
            GetWindowThreadProcessId(window, &process);
            if (process != GetCurrentProcessId()) return TRUE;
            wchar_t name[32]{};
            GetClassNameW(window, name, 32);
            if (!IsWindowVisible(window) || std::wstring_view(name) != L"#32770") return TRUE;
            self->m_Saw.store(true);
            if (HWND button = GetDlgItem(window, IDCANCEL)) {
                PostMessageW(window, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED),
                    reinterpret_cast<LPARAM>(button));
            } else {
                PostMessageW(window, WM_CLOSE, 0, 0);
            }
            return TRUE;
        }

        std::atomic<bool> m_Finished{false};
        std::atomic<bool> m_Saw{false};
        std::thread m_Thread;
    };

}
