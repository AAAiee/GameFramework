#include "pch.h"
#include "EntityTagsCheck.h"
#include "EditorDriver.h"
#include "Panels/SceneHierachyPanel.h"
#include "Rubber/Input/InputOverlay.h"
#include "Rubber/imGui/ItemProbe.h"
#include <imgui_internal.h>
#include <stdexcept>

namespace Rubber {
    namespace {
        void require(bool condition, const char* message) {
            if (!condition) throw std::runtime_error(message);
        }

        bool present(std::string_view path) {
            return ItemProbe::find(path).has_value();
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
                ItemProbe::enable();
            }
            ~TagImGuiScope() {
                ItemProbe::disable();
                InputOverlay::clear();
                ImGui::DestroyContext(m_Context);
                ImGui::SetCurrentContext(m_Previous);
            }
        private:
            ImGuiContext* m_Previous;
            ImGuiContext* m_Context;
        };
    }

    /** A genuine panel fixture: no replacement widget, adapter or deletion path.
     * Mouse/menu gestures go through EditorDriver, labels come from the submitted
     * items, and keyboard focus/text events drive the production editing code.
     */
    class EntityTagsFixture {
    public:
        void run() {
            m_Scene = Scene::create();
            m_Entity = m_Scene->createEntity("initial");
            m_Panel.setContext(m_Scene);
            frame();
            frame();
            m_Driver.click("Scene Hierarchy/initial");
            settle();
            require(present("##Tag"), "Row click did not select the entity");
            m_Driver.clickPoint(arrowPoint(rowItem()), 0);
            settle();
            require(opened(rowItem()), "Row arrow did not expand");
            const std::string unicode = "\xe4\xb8\xad\xe6\x96\x87 caf\xc3\xa9 \xce\xa9";
            const std::string longTag = std::string(8192, 'L') + unicode + " % %s %n tail";
            const std::vector<std::string> values = {
                "%", "%s", "%n", "literal % %s %n", unicode,
                std::string(255, 'a'), std::string(256, 'b'), longTag, "", "renamed %s %n"
            };
            for (const auto& expected : values) {
                edit(expected);
                require(m_Entity.getComponent<TagComponent>().tag == expected,
                    "Production tag input truncated or changed literal UTF-8 bytes");
                require(present("##Tag"), "Rename changed selection");
                observeRow(expected);
            }
            std::cout << "OBSERVED: real hierarchy input retained empty, percent, UTF-8, 255/256 and 8192+ byte tags; labels and IDs checked\n";

            editComponents();

            // Deselect via blank hierarchy space, then select the renamed row.
            auto* hierarchy = ImGui::FindWindowByName("Scene Hierarchy");
            require(hierarchy != nullptr, "Hierarchy window missing");
            m_Driver.clickPoint(ImVec2(hierarchy->InnerRect.Min.x + 24,
                hierarchy->InnerRect.Max.y - 24), 0);
            settle();
            require(!present("##Tag"), "Blank hierarchy click did not deselect");
            m_Driver.click("Scene Hierarchy/renamed %s %n");
            settle();
            require(present("##Tag"), "Renamed row could not be selected");
            m_Driver.click("Scene Hierarchy/renamed %s %n", 1);
            settle(2);
            m_Driver.click("Delete Entity");
            settle();
            const auto handle = static_cast<entt::entity>(m_Entity);
            require(!m_Scene->getRegistry().valid(handle), "Delete Entity menu left the entity alive");
            require(!present("##Tag"), "Deletion left a borrowed selection");
            frame();
            std::cout << "OBSERVED: renamed row retained expansion and selection; blank-space deselection, reselection and Delete Entity menu passed\n";
            m_Entity = {};
            m_Scene.reset();
        }

    private:
        /** Render complete production windows at fixed positions on the private context. */
        void frame() {
            require(++m_Frames < 800, "Tag fixture exceeded frame bound");
            m_Driver.tick();
            InputOverlay::applyToImGui();
            ImGui::NewFrame();
            ItemProbe::beginFrame();
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

        void settle(int extra = 1) {
            unsigned guard = 0;
            while (m_Driver.busy()) {
                require(++guard < 64, "Driver action did not finish");
                frame();
            }
            for (int i = 0; i < extra; ++i) frame();
        }

        UiItem rowItem() const {
            std::optional<UiItem> row;
            for (const auto& item : ItemProbe::snapshot()) {
                if (item.window != "Scene Hierarchy") continue;
                if (!(item.statusFlags & ImGuiItemStatusFlags_Openable)) continue;
                require(!row.has_value(), "Hierarchy submitted more than one openable row");
                row = item;
            }
            require(row.has_value(), "Hierarchy row item missing");
            return *row;
        }

        static bool opened(const UiItem& row) {
            return (row.statusFlags & ImGuiItemStatusFlags_Opened) != 0;
        }

        static ImVec2 arrowPoint(const UiItem& row) {
            return ImVec2(row.min.x + ImGui::GetFontSize() * 0.5f,
                (row.min.y + row.max.y) * 0.5f);
        }

        void edit(const std::string& text) {
            m_Driver.click("##Tag");
            settle();
            auto* property = ImGui::FindWindowByName("Property Channel");
            require(property != nullptr, "Inspector window missing");
            const ImGuiID header = property->GetID(reinterpret_cast<void*>(typeid(TagComponent).hash_code()));
            const ImGuiID input = ImHashStr("##Tag", 0, header);
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
            m_Driver.typeText(text);
            frame();
            io.AddKeyEvent(ImGuiKey_Enter, true);
            frame();
            io.AddKeyEvent(ImGuiKey_Enter, false);
            frame();
            settle();
        }

        /** Drive the production Inspector menus and check registry effects in all builds. */
        void editComponents() {
            require(!m_Entity.hasComponent<CameraComponent>()
                && !m_Entity.hasComponent<VisibilityControlComponent>(),
                "Inspector fixture must start without the components it adds");
            addComponent("Camera");
            require(m_Entity.hasComponent<CameraComponent>(),
                "Inspector Add Component / Camera did not add a camera");
            const bool wasPrimary = m_Entity.getComponent<CameraComponent>().isPrimary;
            m_Driver.click("Primary");
            settle();
            require(m_Entity.getComponent<CameraComponent>().isPrimary == !wasPrimary,
                "Inspector Primary checkbox did not edit the camera");

            addComponent("Visibility");
            require(m_Entity.hasComponent<VisibilityControlComponent>()
                && m_Entity.getComponent<VisibilityControlComponent>().isVisible,
                "Inspector Add Component / Visibility did not add a visible component");
            m_Driver.click("isVisible");
            settle();
            require(!m_Entity.getComponent<VisibilityControlComponent>().isVisible,
                "Inspector visibility checkbox did not hide the entity");
            m_Driver.click("isVisible");
            settle();
            require(m_Entity.getComponent<VisibilityControlComponent>().isVisible,
                "Inspector visibility checkbox did not restore visibility");
            require(present("##Tag"), "Inspector component editing changed hierarchy selection");
            Application::getEventManager()->dispatchAllEvents();
            Application::getEventManager()->dispatchAllEvents();
            std::cout << "OBSERVED: Inspector menus added Camera and Visibility; Primary and visibility checkboxes edited components through production widgets\n";
        }

        /** Open the real Add Component popup and click its labeled menu item. */
        void addComponent(std::string_view name) {
            m_Driver.click("Add Component");
            settle();
            m_Driver.click(name);
            settle();
        }

        /** Observe the submitted row item: exact literal bytes, stable ID, still expanded. */
        void observeRow(const std::string& expected) {
            const UiItem row = rowItem();
            if (!m_EvidenceID) m_EvidenceID = row.id;
            require(row.id == m_EvidenceID, "Submitted row ID changed on rename");
            require(row.label == expected, "Rendered hierarchy label did not contain exact literal tag");
            require(opened(row), "Rename lost row expansion state");
            require(m_Entity.getComponent<TagComponent>().tag == expected, "Label display mutated tag");
        }

        Ref<Scene> m_Scene;
        Entity m_Entity;
        SceneHierachyPanel m_Panel;
        EditorDriver m_Driver;
        ImGuiID m_EvidenceID = 0;
        unsigned m_Frames = 0;
    };

    void checkEntityTags() {
        TagImGuiScope context;
        EntityTagsFixture fixture;
        fixture.run();
    }
}
