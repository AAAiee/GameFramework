#include <pch.h>
#include "Rubber/Scene/Utili/Platform/PlatformUtil.h"
#include "Rubber/Window/Window.h"
#include "Rubber/Core/Application.h"

#include <commdlg.h>
#include <iterator>
#define GLFW_EXPOSE_NATIVE_WIN32 
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Rubber {

    std::wstring fileDialog::openFile(const wchar_t* filter) {
        OPENFILENAMEW ofn;
        WCHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::getWindow().getNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = static_cast<DWORD>(std::size(szFile));
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&ofn) == TRUE)
            return std::wstring(ofn.lpstrFile);
        return {};
    }
    std::wstring fileDialog::saveFile(const wchar_t* filter) {
        OPENFILENAMEW ofn;
        WCHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::getWindow().getNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nFilterIndex = 1;
        ofn.nMaxFile = static_cast<DWORD>(std::size(szFile));
        ofn.lpstrFilter = filter;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (GetSaveFileNameW(&ofn) == TRUE)
            return std::wstring(ofn.lpstrFile);
        return {};
    }
}
