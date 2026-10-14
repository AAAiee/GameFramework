#include <pch.h>
#include "Scene.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Core/Application.h"
#include "Rubber/Event/EventManager.h"
#include "RUbber/Event/AppEvent.h"
#include "Rubber/Renderer/Renderer2D.h"
#include "Rubber/Scene/Utili/Entity.h"
#include "Rubber/Scene/System/RenderSystem.h"
#include "Rubber/Scene/System/WindowSystem.h"

namespace Rubber {


	Entity Scene::createEntity(std::string_view tag)
	{
		entt::entity entityHandler =  m_Registry.create();
		Entity entity(entityHandler, this);

		//every entity has a default tag component
		std::string_view tagText = (tag.empty()) ? "Unknown Entity" : tag;
		entity.addComponent<TagComponent>(tagText);
		return entity;
	}

	void Scene::removeEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	entt::registry& Scene::getRegistry()
	{
		return m_Registry;
	}

	void Scene::updatesViewportSize(glm::vec2 dimensions)
	{
		RB_CORE_ASSERT(dimensions.x * dimensions.y > 0.0f, "invalid dimensions");
		m_ViewPortDimension = dimensions;
	}

	Scene::Scene() {
		systemsInit();
	}

	Scene::~Scene() {
		RB_INFO("Scene Destroyed, shutting down all the systems");
		systemShutDown();
	}

	void Scene::systemsInit()
	{
		m_Systems.emplace_back(makeScope<WindowSystem>());
		m_Systems.emplace_back(makeScope<RendererSystem>());

		for (auto& system : m_Systems) {
			system->init(this);
		}
	}

	void Scene::systemShutDown()
	{
		for (auto& system : m_Systems) {
			system->shutdown();
		}
		m_Systems.clear();
	}


	void Scene::onSystemsUpdate(float ts)
	{
		for (auto& systemPtr : m_Systems) {
			RB_CORE_ASSERT(systemPtr->hasInit(), "system must be initialized before update");
			systemPtr->onUpdate(ts);
		}
	}

	void Scene::onSceneUpdate(float ts)
	{
		onSystemsUpdate(ts);
	}

	Ref<Scene> Scene::create()
	{
		return Ref<Scene>(new Scene());
	}


	template<typename T>
	void Rubber::Scene::onComponentConstruct(Entity ent)
	{
		static_assert(!std::is_same_v<T, T>, "Unsupported scene component construction");
	}


	template<>
	void Rubber::Scene::onComponentConstruct<TagComponent>(Entity ent)
	{
	}


	template<>
	void Rubber::Scene::onComponentConstruct<SpriteComponent>(Entity ent)
	{
	}

	template<>
	void Rubber::Scene::onComponentConstruct<CameraComponent>(Entity ent)
	{
		publishViewportResize();
	}

	template<>
	void Rubber::Scene::onComponentConstruct<TransformComponent>(Entity ent)
	{
	}

	template<>
	void Rubber::Scene::onComponentConstruct<VisibilityControlComponent>(Entity ent)
	{
	}



	template<typename T>
	void Scene::onComponentDelete(Entity ent)
	{
		static_assert(!std::is_same_v<T, T>, "Unsupported scene component deletion");
	}

	template<>
	void Rubber::Scene::onComponentDelete<TagComponent>(Entity ent)
	{
	}


	template<>
	void Rubber::Scene::onComponentDelete<SpriteComponent>(Entity ent)
	{
	}

	template<>
	void Rubber::Scene::onComponentDelete<TransformComponent>(Entity ent)
	{
	}

	template<>
	void Rubber::Scene::onComponentDelete<VisibilityControlComponent>(Entity ent)
	{
	}

	template<>
	void Rubber::Scene::onComponentDelete<CameraComponent>(Entity ent)
	{
		publishViewportResize();
	}


	void Scene::publishViewportResize()
	{
		Ref<EventManager> eventManager = Application::getEventManager();
		// push an viewport resize event whenever a new camera component is added(to be handled by the WindowSystem)
		eventManager->enqueue<ViewPortResizeEvent>({ (uint32_t)m_ViewPortDimension.x, (uint32_t)m_ViewPortDimension.y });
	}

}

