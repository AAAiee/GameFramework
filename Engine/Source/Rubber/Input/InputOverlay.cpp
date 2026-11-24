#include "pch.h"
#include "Rubber/Input/InputOverlay.h"

#include <imgui.h>
#include <unordered_set>

namespace {
    std::unordered_set<int> g_Keys;
    bool g_HasMousePos = false;
    float g_MouseX = 0.0f;
    float g_MouseY = 0.0f;
    bool g_HasMouseButton = false;
    int g_MouseButton = 0;
    bool g_MouseDown = false;
}

void Rubber::InputOverlay::press(int keyCode) {
    g_Keys.insert(keyCode);
}

void Rubber::InputOverlay::release(int keyCode) {
    g_Keys.erase(keyCode);
}

void Rubber::InputOverlay::clear() {
    g_Keys.clear();
    g_HasMousePos = false;
    g_HasMouseButton = false;
}

void Rubber::InputOverlay::setMousePos(float x, float y) {
    g_HasMousePos = true;
    g_MouseX = x;
    g_MouseY = y;
}

void Rubber::InputOverlay::setMouseButton(int button, bool down) {
    g_HasMouseButton = true;
    g_MouseButton = button;
    g_MouseDown = down;
}

void Rubber::InputOverlay::applyToImGui() {
    if (!ImGui::GetCurrentContext()) return;
    ImGuiIO& io = ImGui::GetIO();
    if (g_HasMousePos)
        io.AddMousePosEvent(g_MouseX, g_MouseY);
    if (g_HasMouseButton) {
        io.AddMouseButtonEvent(g_MouseButton, g_MouseDown);
        g_HasMouseButton = false;
    }
}

bool Rubber::InputOverlay::isPressed(int keyCode) {
    return g_Keys.find(keyCode) != g_Keys.end();
}
