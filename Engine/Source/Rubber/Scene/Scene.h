#pragma once  
#include "Rubber/Core/Utility.h"
#include "Rubber/Scene/System/System.h"

#include <entt.hpp>
#include<glm/glm.hpp>
#include <memory>
#include <string_view>


namespace Rubber {  

	class SceneHierachyPanel;
	class Entity;

    /**
     * Owns the entity registry and installed systems. Construction initializes
     * WindowSystem, then RendererSystem; shutdown uses that same order.
     * Requires a live Application, event manager, logger and current GL context.
     * Destroy the scene before those facilities. Other scene systems are dormant.
     * WindowSystem uses a shared subscription name, so simultaneous scenes do
     * not have independent viewport delivery.
     */
	class Scene {  
	public:
        /** Creates an entity with an owned tag; an empty tag becomes "Unknown Entity". */
		Entity createEntity(std::string_view tag="");  
        /** Destroys a valid entity from this scene; borrowed entity/component views expire. */
		void removeEntity(Entity entity);
        /** Returns this scene's borrowed registry; direct mutations bypass component hooks. */
		entt::registry& getRegistry();
		/** Stores usable positive pixel dimensions for later camera construction hooks.
         * Does not update existing cameras; publishViewportResize queues that delivery. */
		void updatesViewportSize(glm::vec2 dimensions);
        /** Queues the stored pixel dimensions for WindowSystem; delivery is deferred. */
		void publishViewportResize();
        /** Shuts down and destroys owned systems before the registry is destroyed. */
		~Scene();
        /** Updates installed systems in insertion order; ts is elapsed seconds. */
		void onSceneUpdate(float ts);
        /** Installs and initializes WindowSystem then RendererSystem; do not call twice. */
	    void systemsInit();
        /** Shuts down installed systems in insertion order and releases their ownership. */
	    void systemShutDown();
        /** Calls onUpdate(ts) on initialized systems in insertion order; ts is seconds. */
	    void onSystemsUpdate(float ts);

	private:
		Scene();
		/** Runs before registry insertion; only explicitly specialized types are supported.
         * Primary definitions live in Scene.cpp; unsupported types instantiated
         * elsewhere may fail at link time rather than produce the dependent assertion.
         */
		template<typename T>
		void onComponentConstruct(Entity ent);
		/** Runs before registry removal; only explicitly specialized types are supported.
         * Primary definitions live in Scene.cpp; unsupported types instantiated
         * elsewhere may fail at link time rather than produce the dependent assertion.
         */
		template<typename T>
		void onComponentDelete(Entity ent);
	public:  
        /** Returns shared ownership of a scene with its systems already initialized. */
		static Ref<Scene> create();  

	private:  
		entt::registry m_Registry;
		Vector<Scope<SystemBase>> m_Systems;
		glm::vec2 m_ViewPortDimension = { 1600.0f, 900.0f };

        friend class ViewportPixelsFixture; // First-party propagation oracle.
		friend class Entity;
		friend class SceneHierachyPanel;
		friend class SceneSerializer;
	};  

	

}
