#include <EditorLayer.h>
#include <cmath>
#include <limits>
#include <optional>

namespace {
    // Truncate once, before comparison or publication. Reject before float-to-int conversion.
    std::optional<glm::uvec2> viewportPixels(glm::vec2 available) {
        const double limit = (std::numeric_limits<uint32_t>::max)();
        if (!std::isfinite(available.x) || !std::isfinite(available.y)
            || available.x < 1.0f || available.y < 1.0f
            || available.x > limit || available.y > limit) return std::nullopt;
        return glm::uvec2(static_cast<uint32_t>(available.x), static_cast<uint32_t>(available.y));
    }
}

namespace Rubber {
	EditorLayer::EditorLayer() :Layer("example") { }

	void EditorLayer::onAttach()
	{
		auto io = ImGui::GetIO();

		FontInfo info;
		info.name = "OpenSans-Bold";
		info.path = "assets/fonts/OpenSans/static/OpenSans-Bold.ttf";
		info.size = 18.0f;

		// load fonts
		FontManager::loadFontFromTTF(info);
		info.name = "OpenSans-Regular";
		info.path = "assets/fonts/OpenSans/static/OpenSans-Regular.ttf";
		FontManager::loadFontFromTTF(info);

		// set default font to OpenSans 
		FontManager::setDefaultFont("OpenSans-Regular");

		uint32_t width = Application::getWindow().getWidth();
		uint32_t height = Application::getWindow().getHeight();

		m_FrameBuffer = FrameBuffer::create({ width,height,1,false });
		m_ActiveScene = Scene::create();
        initializeSceneViewport();
		m_SceneHierachyPanel.setContext(m_ActiveScene);


		Application::getEventManager()->subscribe<KeyPressedEvent>("Editor layer shortcut key event", [this](const KeyPressedEvent& e) {
			return shortcutCallBack(e);
		});
	}

	void EditorLayer::onDetach()
	{
		Application::getEventManager()->unsubscribe<KeyPressedEvent>("Editor layer shortcut key event");
	}

	void EditorLayer::onUpdate(const float ts)
	{
        // The previous ImGui frame supplies available logical dimensions.
        const auto pixels = viewportPixels(m_ViewPortDimension);
        const auto& spec = m_FrameBuffer->getSpecification();
        if (pixels && (spec.m_Width != pixels->x || spec.m_Height != pixels->y)) {
            m_FrameBuffer->resize(pixels->x, pixels->y);
            Application::getEventManager()->enqueue(ViewPortResizeEvent(pixels->x, pixels->y));
            m_ActiveScene->updatesViewportSize(glm::vec2(*pixels));
        }

		// Update all systems and render everything into the frame buffer
		m_FrameBuffer->bind();
		Renderer2D::resetRendererStat();
		m_ActiveScene->onSceneUpdate(ts);
		m_FrameBuffer->unbind();
	}

	void EditorLayer::onImGuiRender()
	{
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static bool dockSpaceOpen = true;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockSpaceOpen, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace

		// config the minimum window width for any docked windows
		ImGuiStyle& style = ImGui::GetStyle();
        const float windowMinimumSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 420.0f;
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		// the rest of windows' minimum width stay what it was
		style.WindowMinSize.x = windowMinimumSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("New", "Ctrl+N")) {
					newScene();
				}

				if (ImGui::MenuItem("Open...", "Ctrl+O")) {
					openScene();
				}

				if (ImGui::MenuItem("Save As...")) {
					saveScene();
				}

				if (ImGui::MenuItem("Exit")) {
					Application::close();
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// Stats Panel 
		ImGui::Begin("Setting");
		ImGui::Text("DrawCount: %d", Renderer2D::getDrawCallCount());
		ImGui::Text("QuadCount: %d", Renderer2D::getQuadCount());
		ImGui::Text("IndexCount: %d", Renderer2D::getIndexCount());
		ImGui::Text("VertexCount: %d", Renderer2D::getVertexCount());
		ImGui::Text("Frame Time: %f", Application::getTimer()->getAverageFrameTime());
		ImGui::Text("Current FPS : %d", Application::getTimer()->getFps());
		ImGui::End();

		m_SceneHierachyPanel.onImGuiRender();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f,0.0f });
		ImGui::Begin("ViewPort");

		//TODO:: Only Enable zoom callback when is focused or hovered
		m_IsViewPortFocused = ImGui::IsWindowFocused();
		m_IsViewPortHovered = ImGui::IsWindowHovered();

		ImVec2 curViewPortSize = ImGui::GetContentRegionAvail();
		m_ViewPortDimension = { curViewPortSize.x, curViewPortSize.y };
		ImGui::Image(reinterpret_cast<void*>((uint64_t)m_FrameBuffer->getColorAttachmentID()), ImVec2(m_ViewPortDimension.x, m_ViewPortDimension.y), { 0,1 }, { 1,0 });
		ImGui::End();
		ImGui::PopStyleVar();


		ImGui::End();
	}

    void EditorLayer::initializeSceneViewport()
    {
        // Before layout, or while collapsed, retain the last usable render target.
        const auto& spec = m_FrameBuffer->getSpecification();
        const auto pixels = viewportPixels(m_ViewPortDimension).value_or(
            glm::uvec2(spec.m_Width, spec.m_Height));
        if (pixels.x > 0 && pixels.y > 0)
            m_ActiveScene->updatesViewportSize(glm::vec2(pixels));
    }

	void EditorLayer::newScene()
	{
		m_ActiveScene.reset();
		m_ActiveScene = Scene::create();
		m_SceneHierachyPanel.setContext(m_ActiveScene);
		initializeSceneViewport();
	}

	void EditorLayer::openScene()
	{
		std::wstring filepath = fileDialog::openFile(L"Rubber Scene (*.rubber)\0*.rubber\0\0");
		if (!filepath.empty()) {
			m_ActiveScene.reset();
			m_ActiveScene = Scene::create();
			m_SceneHierachyPanel.setContext(m_ActiveScene);
			initializeSceneViewport();

			SceneSerializer serializer(m_ActiveScene);
			serializer.deserialize(filepath);
		}
	}

	void EditorLayer::saveScene()
	{
		std::wstring filepath = fileDialog::saveFile(L"Rubber Scene (*.rubber)\0*.rubber\0\0");
		if (!filepath.empty()) {
			SceneSerializer serializer(m_ActiveScene);
			serializer.serialize(filepath);
		}
	}

	bool EditorLayer::shortcutCallBack(const KeyPressedEvent& e)
	{
		if (e.getRepeatCount() > 0) return false; // false if it is a repeat event

		int32_t keyPressed = e.getKeyCode();
		bool isControlPressed = Input::isKeyPressed(RB_KEY_LEFT_CONTROL) || Input::isKeyPressed(RB_KEY_RIGHT_CONTROL);

		switch (keyPressed) {
			case RB_KEY_O:
			{
				if (isControlPressed) {
					openScene();
					return true;
				}
				break;
			}
			case RB_KEY_N:
			{
				if (isControlPressed) {
					newScene();
					return true;
				}
				break;
			}

			case RB_KEY_S:
			{
				if (isControlPressed) {
					saveScene();
					return true;
				}
				break;
			}
		}
		return false;
	}
}


