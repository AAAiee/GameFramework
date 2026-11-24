#include "pch.h"
#include "EditorDriver.h"
#include "Rubber/Input/InputOverlay.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ostream>
#include <stdexcept>
#include <sstream>

namespace Rubber {
    namespace {
        void require(bool condition, const std::string& message) {
            if (!condition) throw std::runtime_error(message);
        }
    }

    void EditorDriver::click(std::string_view path, int button, int occurrence) {
        Action action;
        action.kind = Kind::Click;
        action.path = std::string(path);
        action.button = button;
        action.occurrence = occurrence;
        m_Queue.push_back(std::move(action));
    }

    void EditorDriver::clickPoint(ImVec2 point, int button) {
        Action action;
        action.kind = Kind::ClickPoint;
        action.point = point;
        action.button = button;
        m_Queue.push_back(std::move(action));
    }

    void EditorDriver::rightClickBlank(std::string_view window) {
        Action action;
        action.kind = Kind::RightClickBlank;
        action.path = std::string(window);
        action.button = 1;
        m_Queue.push_back(std::move(action));
    }

    void EditorDriver::drag(std::string_view path, ImVec2 delta, int occurrence) {
        Action action;
        action.kind = Kind::Drag;
        action.path = std::string(path);
        action.delta = delta;
        action.occurrence = occurrence;
        m_Queue.push_back(std::move(action));
    }

    void EditorDriver::dragPoint(ImVec2 start, ImVec2 delta) {
        Action action;
        action.kind = Kind::DragPoint;
        action.delta = delta;
        action.point = start;
        m_Queue.push_back(std::move(action));
    }

    void EditorDriver::waitFrames(int frames) {
        Action action;
        action.kind = Kind::Wait;
        action.wait = frames;
        m_Queue.push_back(std::move(action));
    }

    void EditorDriver::dump(std::ostream& out) const {
        ItemProbe::dump(out);
    }

    UiItem EditorDriver::requireItem(const Action& action) const {
        auto item = ItemProbe::find(action.path, action.occurrence);
        if (!item) {
            std::ostringstream message;
            message << "UI item not found: " << action.path
                << " occurrence=" << action.occurrence << "\n";
            ItemProbe::dump(message);
            throw std::runtime_error(message.str());
        }
        return *item;
    }

    ImVec2 EditorDriver::blankPoint(std::string_view window) const {
        ImGuiWindow* imguiWindow = ImGui::FindWindowByName(std::string(window).c_str());
        require(imguiWindow != nullptr && imguiWindow->Active,
            "Window missing for blank click: " + std::string(window));
        const ImRect inner = imguiWindow->InnerRect;
        ImVec2 point(inner.Min.x + 32.0f, inner.Max.y - 36.0f);
        if (point.y < inner.Min.y + 8.0f)
            point.y = (inner.Min.y + inner.Max.y) * 0.5f;
        if (point.x > inner.Max.x - 8.0f)
            point.x = (inner.Min.x + inner.Max.x) * 0.5f;
        return point;
    }

    void EditorDriver::mouseMove(ImVec2 point) {
        InputOverlay::setMousePos(point.x, point.y);
    }

    void EditorDriver::mouseButton(int button, bool down) {
        InputOverlay::setMouseButton(button, down);
    }

    void EditorDriver::tick() {
        if (m_Queue.empty()) return;
        Action& action = m_Queue.front();
        switch (action.kind) {
            case Kind::Wait:
                if (--action.wait <= 0) m_Queue.erase(m_Queue.begin());
                return;
            case Kind::Click:
            case Kind::ClickPoint: {
                const ImVec2 point = action.kind == Kind::Click
                    ? ItemProbe::center(requireItem(action))
                    : action.point;
                if (action.phase == 0) { mouseMove(point); action.phase = 1; return; }
                if (action.phase == 1) { mouseButton(action.button, true); action.phase = 2; return; }
                if (action.phase == 2) { mouseButton(action.button, false); action.phase = 3; return; }
                m_Queue.erase(m_Queue.begin());
                return;
            }
            case Kind::RightClickBlank: {
                const ImVec2 point = blankPoint(action.path);
                if (action.phase == 0) { mouseMove(point); action.phase = 1; return; }
                if (action.phase == 1) { mouseButton(1, true); action.phase = 2; return; }
                if (action.phase == 2) { mouseButton(1, false); action.phase = 3; return; }
                m_Queue.erase(m_Queue.begin());
                return;
            }
            case Kind::Drag:
            case Kind::DragPoint: {
                ImVec2 start = action.point;
                if (action.kind == Kind::Drag) start = ItemProbe::center(requireItem(action));
                const ImVec2 end(start.x + action.delta.x, start.y + action.delta.y);
                if (action.phase == 0) { mouseMove(start); action.phase = 1; return; }
                if (action.phase == 1) { mouseButton(0, true); action.phase = 2; return; }
                if (action.phase == 2) { mouseMove(end); action.phase = 3; return; }
                if (action.phase == 3) { mouseButton(0, false); action.phase = 4; return; }
                m_Queue.erase(m_Queue.begin());
                return;
            }
        }
    }
}
