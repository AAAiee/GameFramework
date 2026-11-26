#pragma once

#include "Rubber/imGui/ItemProbe.h"
#include "NativeDialog.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Rubber {

    /** Queues production ImGui mouse/text gestures for the next Application frames.
     * Call tick() after EditorLayer::onImGuiRender so the snapshot is this frame's
     * items and IO events apply at the next NewFrame.
     */
    class EditorDriver {
    public:
        void click(std::string_view path, int button = 0, int occurrence = 0);
        void clickPoint(ImVec2 point, int button = 0);
        void rightClickBlank(std::string_view window);
        void drag(std::string_view path, ImVec2 delta, int occurrence = 0);
        void dragPoint(ImVec2 start, ImVec2 delta);
        void waitFrames(int frames);
        void typeText(std::string_view text);
        void chord(int key);
        void openCombo(std::string_view path);
        void beginDialogCancel();
        bool dialogCancelPending() const { return m_Dialog != nullptr; }
        bool sawDialog() const;
        void endDialogCancel();
        bool busy() const { return !m_Queue.empty(); }
        void tick();
        void dump(std::ostream& out) const;

    private:
        enum class Kind { Click, ClickPoint, RightClickBlank, Drag, DragPoint, Wait, Text, Chord };

        struct Action {
            Kind kind = Kind::Click;
            std::string path;
            std::string text;
            int button = 0;
            int occurrence = 0;
            int key = 0;
            ImVec2 delta{};
            ImVec2 point{};
            int wait = 0;
            int phase = 0;
        };

        UiItem requireItem(const Action& action) const;
        ImVec2 blankPoint(std::string_view window) const;
        void mouseMove(ImVec2 point);
        void mouseButton(int button, bool down);

        std::vector<Action> m_Queue;
        std::unique_ptr<NativeDialog::CancelDialogSession> m_Dialog;
    };

}
