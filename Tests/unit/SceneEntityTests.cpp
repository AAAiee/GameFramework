// Scene and Entity behavior on a system-less scene: tag defaults, add/remove
// for the Inspector-backed component set, the camera hook that must not need
// an Application, and handle invalidation on entity removal. Dormant-system
// components (Input/Animation/Collision/Move/Script/Fsm) are not instantiated;
// the Entity API does not support them.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include <doctest.h>

#include "Rubber/Event/EventManager.h"
#include "Rubber/Scene/Scene.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Scene/Utili/Entity.h"

#include <glm/glm.hpp>

namespace {

    // Camera construction/removal hooks publish viewport resizes onto the
    // process-wide event manager singleton. Drain that queue (promote, then
    // deliver to the no subscribers a unit run has) so this file leaves the
    // shared queue empty for later cases.
    void drainQueuedViewportEvents() {
        auto eventManager = Rubber::EventManager::create();
        eventManager->dispatchAllEvents();
        eventManager->dispatchAllEvents();
    }

} // namespace

TEST_CASE("scene-createWithoutSystems-supports-registry-work") {
    auto scene = Rubber::Scene::createWithoutSystems();
    REQUIRE(scene);

    SUBCASE("an empty tag becomes Unknown Entity") {
        Rubber::Entity defaultTag = scene->createEntity();
        CHECK(defaultTag.getComponent<Rubber::TagComponent>().tag == "Unknown Entity");

        Rubber::Entity emptyTag = scene->createEntity("");
        CHECK(emptyTag.getComponent<Rubber::TagComponent>().tag == "Unknown Entity");

        Rubber::Entity named = scene->createEntity("Hero");
        CHECK(named.getComponent<Rubber::TagComponent>().tag == "Hero");
    }

    SUBCASE("tag components remove and re-add through the hook") {
        Rubber::Entity entity = scene->createEntity("Tagged");
        CHECK(entity.hasComponent<Rubber::TagComponent>());

        entity.removeComponent<Rubber::TagComponent>();
        CHECK(!entity.hasComponent<Rubber::TagComponent>());

        entity.addComponent<Rubber::TagComponent>("Renamed");
        CHECK(entity.getComponent<Rubber::TagComponent>().tag == "Renamed");
    }

    SUBCASE("transform sprite and visibility add remove and read back") {
        Rubber::Entity entity = scene->createEntity("Composed");

        CHECK(!entity.hasComponent<Rubber::TransformComponent>());
        auto& transform = entity.addComponent<Rubber::TransformComponent>();
        transform.position = { 1.0f, 2.0f, 3.0f };
        CHECK(entity.hasComponent<Rubber::TransformComponent>());
        CHECK(entity.getComponent<Rubber::TransformComponent>().position == glm::vec3(1.0f, 2.0f, 3.0f));
        entity.removeComponent<Rubber::TransformComponent>();
        CHECK(!entity.hasComponent<Rubber::TransformComponent>());

        auto& sprite = entity.addComponent<Rubber::SpriteComponent>();
        sprite.color = { 0.25f, 0.5f, 0.75f, 1.0f };
        CHECK(entity.hasComponent<Rubber::SpriteComponent>());
        CHECK(entity.getComponent<Rubber::SpriteComponent>().color == glm::vec4(0.25f, 0.5f, 0.75f, 1.0f));
        entity.removeComponent<Rubber::SpriteComponent>();
        CHECK(!entity.hasComponent<Rubber::SpriteComponent>());

        entity.addComponent<Rubber::VisibilityControlComponent>().isVisible = false;
        CHECK(entity.hasComponent<Rubber::VisibilityControlComponent>());
        CHECK(entity.getComponent<Rubber::VisibilityControlComponent>().isVisible == false);
        entity.removeComponent<Rubber::VisibilityControlComponent>();
        CHECK(!entity.hasComponent<Rubber::VisibilityControlComponent>());
    }

    SUBCASE("camera hooks run without an Application singleton") {
        // Before the publishViewportResize seam, adding a camera dereferenced
        // the null Application singleton. The hook now uses the process-wide
        // event manager, which exists without any Application.
        Rubber::Entity entity = scene->createEntity("CameraEntity");
        CHECK(!entity.hasComponent<Rubber::CameraComponent>());

        auto& camera = entity.addComponent<Rubber::CameraComponent>();
        camera.isPrimary = true;
        CHECK(entity.hasComponent<Rubber::CameraComponent>());
        CHECK(entity.getComponent<Rubber::CameraComponent>().isPrimary);

        entity.removeComponent<Rubber::CameraComponent>();
        CHECK(!entity.hasComponent<Rubber::CameraComponent>());
    }

    SUBCASE("removeEntity invalidates the removed handle only") {
        Rubber::Entity doomed = scene->createEntity("Doomed");
        doomed.addComponent<Rubber::TransformComponent>();
        Rubber::Entity survivor = scene->createEntity("Survivor");

        scene->removeEntity(doomed);
        CHECK(!doomed.hasComponent<Rubber::TagComponent>());
        CHECK(!doomed.hasComponent<Rubber::TransformComponent>());

        CHECK(survivor.hasComponent<Rubber::TagComponent>());
        CHECK(survivor.getComponent<Rubber::TagComponent>().tag == "Survivor");
    }

    drainQueuedViewportEvents();
}
