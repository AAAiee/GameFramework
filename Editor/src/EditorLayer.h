#pragma once

#include"Panels/SceneHierachyPanel.h"
#include "Utilities/FontsManager/FontManager.h"
#include "Rubber/Scene/Utili/Serializer/SceneSerializer.h"
#include "Rubber/Scene/Utili/Platform/PlatformUtil.h"

#include<Rubber.h>

#include <glm/glm.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Rubber {

	class KeyPressedEvent;

    /** Owns one scene and render target; requires Application, GL and ImGui contexts.
     * Application dispatches events, performs fixed updates, then renders ImGui.
     * onUpdate consumes the preceding panel size, resizes to usable integer pixels,
     * queues camera propagation and renders the scene into the framebuffer.
     * onImGuiRender records the next size and displays the current attachment.
     * Independent simultaneous scene viewports are unsupported.
     */
	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		/** Loads fonts and creates scene/target after ImGui attachment; subscribes shortcuts. */
		void onAttach() override;
		/** Unsubscribes shortcuts; Application destruction does not automatically invoke this hook. */
		void onDetach();
		/** ts is fixed elapsed seconds; uses last usable pixel dimensions for scene rendering. */
		void onUpdate(const float ts) override;
		/** Requires an active ImGui frame; presents target and edits scene through panels. */
		void onImGuiRender() override;

        Ref<Scene> activeScene() const { return m_ActiveScene; }
        glm::vec2 viewPortDimension() const { return m_ViewPortDimension; }

        /** Grants the first-party runtime fixture access to the active editor scene. */
        friend class EditorIssue12Fixture;
        /** Drives real scene Save As/Open and native dialogs in Regression. */
        friend class EditorIssue15Fixture;
        friend class ViewportPixelsFixture;

	private:
        void initializeSceneViewport();
		void newScene();
		void openScene();
        /** Dialog-free open; resets the scene and loads filepath's YAML. */
        void openSceneFrom(const std::filesystem::path& filepath);
		void saveScene();
        /** Dialog-free save; writes the active scene's YAML to filepath. */
        void saveSceneTo(const std::filesystem::path& filepath);
		bool shortcutCallBack(const KeyPressedEvent& e);


	private:
		Ref<FrameBuffer> m_FrameBuffer;
		Ref<Scene> m_ActiveScene;
		SceneHierachyPanel m_SceneHierachyPanel;
		Entity m_SquareEntity;
		Entity m_SecondCamera;
		Entity m_Camera;

		glm::vec2 m_ViewPortDimension{};
		bool m_IsViewPortFocused = false;
		bool m_IsViewPortHovered = false;
	};
}


