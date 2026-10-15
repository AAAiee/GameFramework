#include "pch.h"
#include "EntityTagsCheck.h"
#include "Panels/SceneHierachyPanel.h"
#include <imgui_internal.h>
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        /** Own a separate real context; preserve the full suite's editor atlas. */
        class TagImGuiScope {
        public:
            TagImGuiScope() : m_Previous(ImGui::GetCurrentContext()) {
                m_Context = ImGui::CreateContext();
                auto& io = ImGui::GetIO();
                io.IniFilename = nullptr;
                io.DisplaySize = ImVec2(1600, 900);
                io.DeltaTime = 1.0f / 60.0f;
                unsigned char* pixels = nullptr;
                int width = 0, height = 0;
                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                require(pixels && width > 0 && height > 0, "Tag fixture font atlas failed");
            }
            ~TagImGuiScope() {
                ImGui::DestroyContext(m_Context);
                ImGui::SetCurrentContext(m_Previous);
            }
        private:
            ImGuiContext* m_Previous;
            ImGuiContext* m_Context;
        };
    }

    /** A genuine panel fixture: no replacement widget, adapter or deletion path.
     * Internal ImGui access observes IDs/logged labels and requests keyboard focus.
     * Mouse/key/text events drive the production selection, editing and menu code.
     */
    class EntityTagsFixture {
    public:
        void run() {
            m_Scene = Scene::create();
            m_Entity = m_Scene->createEntity("initial");
            m_Entity.getComponent<TagComponent>().tag.clear();
            m_Panel.setContext(m_Scene);
            frame();
            frame();
            click(rowPoint(), 0);
            require(m_Panel.m_SelectionContext == m_Entity, "Row click did not select the entity");
            auto* hierarchy = ImGui::FindWindowByName("Scene Hierarchy");
            const ImGuiID rowID = hierarchy->GetID(reinterpret_cast<void*>(static_cast<uintptr_t>(m_Entity)));
            // Opening the arrow also checks that expansion survives tag edits.
            click(ImVec2(hierarchy->DC.CursorStartPos.x + ImGui::GetFontSize() * 0.5f,
                hierarchy->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f), 0);
            require(hierarchy->StateStorage.GetInt(rowID, 0) != 0, "Row arrow did not expand");
            const std::string unicode = "\xe4\xb8\xad\xe6\x96\x87 caf\xc3\xa9 \xce\xa9";
            const std::string longTag = std::string(8192, 'L') + unicode + " % %s %n tail";
            const std::vector<std::string> values = {
                "%", "%s", "%n", "literal % %s %n", unicode,
                std::string(255, 'a'), std::string(256, 'b'), longTag, "", "renamed %s %n"
            };
            observeLabel("");
            for (const auto& expected : values) {
                edit(expected);
                require(m_Entity.getComponent<TagComponent>().tag == expected,
                    "Production tag input truncated or changed literal UTF-8 bytes");
                require(m_Panel.m_SelectionContext == m_Entity, "Rename changed selection");
                observeLabel(expected);
                require(hierarchy->GetID(reinterpret_cast<void*>(static_cast<uintptr_t>(m_Entity))) == rowID,
                    "Rename changed the hierarchy row ID");
                require(hierarchy->StateStorage.GetInt(rowID, 0) != 0,
                    "Rename lost row expansion state");
            }
            std::cout << "OBSERVED: real hierarchy input retained empty, percent, UTF-8, 255/256 and 8192+ byte tags; labels and IDs checked\n";

            // Deselect via blank hierarchy space, then select the renamed row.
            click(ImVec2(hierarchy->Pos.x + 80, hierarchy->Pos.y + 200), 0);
            require(!m_Panel.m_SelectionContext, "Blank hierarchy click did not deselect");
            click(rowPoint(), 0);
            require(m_Panel.m_SelectionContext == m_Entity, "Renamed row could not be selected");
            click(rowPoint(), 1);
            frame();
            require(!GImGui->OpenPopupStack.empty(), "Entity context menu did not open");
            auto* popup = GImGui->OpenPopupStack.back().Window;
            require(popup && popup->Active, "Entity context menu is not active");
            const ImVec2 menuPoint(popup->DC.CursorStartPos.x + 35,
                popup->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f);
            const auto handle = static_cast<entt::entity>(m_Entity);
            click(menuPoint, 0);
            require(!m_Scene->getRegistry().valid(handle), "Delete Entity menu left the entity alive");
            require(!m_Panel.m_SelectionContext, "Deletion left a borrowed selection");
            frame();
            std::cout << "OBSERVED: renamed row retained expansion and selection; blank-space deselection, reselection and Delete Entity menu passed\n";
            m_Entity = {};
            m_Scene.reset();
        }

    private:
        /** Render complete production windows at fixed positions on the private context. */
        void frame() {
            require(++m_Frames < 300, "Tag fixture exceeded frame bound");
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowSize(ImVec2(700, 800));
            ImGui::Begin("Scene Hierarchy");
            ImGui::End();
            ImGui::SetNextWindowPos(ImVec2(730, 10));
            ImGui::SetNextWindowSize(ImVec2(700, 800));
            ImGui::Begin("Property Channel");
            ImGui::End();
            m_Panel.onImGuiRender();
            ImGui::Render();
            require(ImGui::GetDrawData() && ImGui::GetDrawData()->TotalVtxCount > 0,
                "Production panel generated no draw data");
        }

        ImVec2 rowPoint() const {
            auto* window = ImGui::FindWindowByName("Scene Hierarchy");
            require(window != nullptr, "Hierarchy window missing");
            return ImVec2(window->DC.CursorStartPos.x + 80,
                window->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f);
        }

        void click(ImVec2 point, int button) {
            auto& io = ImGui::GetIO();
            io.AddMousePosEvent(point.x, point.y);
            frame();
            io.AddMouseButtonEvent(button, true);
            frame();
            io.AddMouseButtonEvent(button, false);
            frame();
        }

        void edit(const std::string& text) {
            auto* property = ImGui::FindWindowByName("Property Channel");
            const ImGuiID header = property->GetID(reinterpret_cast<void*>(typeid(TagComponent).hash_code()));
            const ImGuiID input = ImHashStr("##Tag", 0, header);
            ImGui::ActivateItem(input);
            GImGui->NavNextActivateFlags = ImGuiActivateFlags_PreferInput;
            frame();
            require(ImGui::GetActiveID() == input && ImGui::GetInputTextState(input),
                "Production tag input did not receive focus");
            auto& io = ImGui::GetIO();
            io.AddKeyEvent(ImGuiMod_Ctrl, true);
            io.AddKeyEvent(ImGuiKey_A, true);
            frame();
            io.AddKeyEvent(ImGuiKey_A, false);
            io.AddKeyEvent(ImGuiMod_Ctrl, false);
            frame();
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            frame();
            io.AddKeyEvent(ImGuiKey_Backspace, false);
            frame();
            require(m_Entity.getComponent<TagComponent>().tag.empty(), "Select-all/backspace did not clear tag");
            io.AddInputCharactersUTF8(text.c_str());
            frame();
            io.AddKeyEvent(ImGuiKey_Enter, true);
            frame();
            io.AddKeyEvent(ImGuiKey_Enter, false);
            frame();
        }

        /** Log the actual TreeNodeEx label and observe its submitted item ID. */
        void observeLabel(const std::string& expected) {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(10, 820));
            ImGui::SetNextWindowSize(ImVec2(1500, 75));
            ImGui::Begin("Tag label evidence");
            ImGui::LogToBuffer();
            m_Panel.drawEntityNode(m_Entity);
            const auto id = GImGui->LastItemData.ID;
            if (!m_EvidenceID) m_EvidenceID = id;
            require(id != 0 && id == m_EvidenceID, "Submitted row ID changed on rename");
            const std::string logged(GImGui->LogBuffer.c_str());
            require(expected.empty() ? logged.empty() : logged.ends_with(expected),
                "Rendered hierarchy label did not contain exact literal tag");
            require(m_Entity.getComponent<TagComponent>().tag == expected, "Label display mutated tag");
            ImGui::LogFinish();
            ImGui::End();
            ImGui::Render();
            frame();
        }

        Ref<Scene> m_Scene;
        Entity m_Entity;
        SceneHierachyPanel m_Panel;
        ImGuiID m_EvidenceID = 0;
        unsigned m_Frames = 0;
    };

    void checkEntityTags() {
        TagImGuiScope context;
        EntityTagsFixture fixture;
        fixture.run();
    }
}
