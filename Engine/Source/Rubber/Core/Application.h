#pragma once
#include "Rubber/Layer/LayerStack.h"

namespace Rubber
{
	//fwd
	class Window;
	class Event;
	class WindowCloseEvent;
	class WindowResizeEvent;
	class GameLoopTimer;
	class EventManager;
	class Camera;
	class ImGuiLayer;
	class AssetManager;


	/** Owns the window, timer and LayerStack; the stack deletes layers still in it.
	 * Static accessors borrow this single live Application's facilities. Ref accessors
	 * share ownership; window/timer references must not outlive the Application.
	 * Initial attachment runs once in reverse stack order, attaching the ImGui overlay
	 * before ordinary layers. Updates and ImGui rendering run forward: layers, then overlays.
	 * Push takes ownership but does not attach; late pushes are not automatically
	 * attached. Do not mutate the stack during traversal. Lifecycle/teardown redesign
	 * and restarting a closed Application are outside this API's contract.
	 */
	class  Application
	{
	public:
		Application(const std::string_view name="");
		virtual ~Application();

	public:
		static Window& getWindow();
		static Ref<EventManager> getEventManager();;
		static const Scope<GameLoopTimer>& getTimer();
		static Ref<AssetManager> getAssetManager();
		static void close();

	public:
		void run();
		void pushLayer(Layer* layer);
		void pushOverlay(Layer* layer);

	private:
		Window& getWindowImpl();
		void attachAll();
		void runFrame(double& lag);
		void updateFixedLayers(double delta, bool previousFrameMinimized, double& lag);
		// First-party checks drive the exact frame and fixed-update policy used by run().
		friend class MinimizeRestoreFixture;

	private:
		bool m_Runing = true;
		//when minimized, stop layer update, (but keep ImGui update)
		bool m_IsWindowMinimized = false;
		//one application-> one instance
		static Application* s_Instance;
		bool m_FirstRun = true;

	private:
		Scope<Window> m_Window;
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;
		Ref<EventManager> m_Em;
		Scope<GameLoopTimer> m_Timer;
		Ref<AssetManager> m_AssetManager;
	
	private:
		bool onWindowClose(const WindowCloseEvent& e);
		bool onWindowResize(const WindowResizeEvent& e);

	private:
		static const float FIXED_TIME_STAMP;
	};
	
}
