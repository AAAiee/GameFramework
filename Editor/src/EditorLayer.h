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

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		void onAttach() override;
		void onDetach();
		void onUpdate(const float ts) override;
		void onImGuiRender() override;

        /** Grants the first-party runtime fixture access to the active editor scene. */
        friend class EditorIssue12Fixture;

	private:
		void newScene();
		void openScene();
		void saveScene();
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


