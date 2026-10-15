#pragma once

#include <Rubber.h>
#include <imgui.h>

namespace Rubber {

    /** Hierarchy/properties view with weak scene ownership. The caller must keep
     * the scene alive for rendering. Selection borrows an Entity and its Scene;
     * setContext clears it. Remove selected entities through the panel or reset
     * context before external deletion; entity validity is not managed here.
     * Tags are UTF-8, null-terminated text (no embedded NUL), displayed literally
     * and edited in their owning std::string without a fixed byte limit. Rendering
     * still depends on the active font's glyph coverage. Row identity uses the
     * entity handle, so renaming retains selection and expansion state.
     */
	class SceneHierachyPanel {
	public:
		SceneHierachyPanel() = default;

		SceneHierachyPanel(const Ref<Scene>& scene);

        /** Borrow a non-null scene weakly and clear the previous selection. */
		void setContext(const Ref<Scene>& scene);

        /** Render in an active ImGui frame; requires a live context Scene and
         * valid tagged entities. UI deletion clears a selected borrowed handle.
         */
		void onImGuiRender();

	private:
        friend class EntityTagsFixture;
		void drawEntityNode(Entity entity);
		void drawComponentNode(Entity entity);


	private:
		weakRef<Scene> m_Context;
		Entity m_SelectionContext; 
	};


}

