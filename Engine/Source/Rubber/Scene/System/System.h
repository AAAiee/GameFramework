#pragma once
#include <entt.hpp>


namespace Rubber {
	class Scene;

    /**
     * Scene-owned system interface. Borrows the scene and registry after init;
     * callers initialize once before updating and shut down before scene death.
     * Derived init/shutdown implementations must call the base operations.
     */
	class SystemBase {
	public:
        /** Borrows a nonnull live scene and its registry; repeat initialization is invalid. */
		virtual void init(Scene* scene);
		virtual ~SystemBase() {};

	    SystemBase() {};

        /** Marks the system uninitialized; borrowed pointers are not cleared or released. */
		virtual void shutdown();

        /** Updates an initialized system; ts is elapsed seconds. The base performs no work. */
		virtual void onUpdate(float ts);
		
        /** Reports the initialization flag, without checking borrowed pointer lifetime. */
		virtual bool hasInit() const { return m_HadInit; }

   protected:
	   Scene* m_ScenePtr = nullptr;
	   entt::registry* m_Registry = nullptr;
	   bool m_HadInit = false;
	};
}
