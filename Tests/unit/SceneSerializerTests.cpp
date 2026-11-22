// SceneSerializer behavior for the fields the Inspector actually writes: tag,
// transform, sprite color, visibility, camera parameters and flags. The entity
// id placeholder in the file format is frozen as-is. Textures and scripts do
// not round trip, and the runtime serialize entry points stay unimplemented;
// neither is driven here.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include <doctest.h>

#include "Rubber/Event/EventManager.h"
#include "Rubber/Scene/Scene.h"
#include "Rubber/Scene/SceneCamera.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Scene/Utili/Entity.h"
#include "Rubber/Scene/Utili/Serializer/SceneSerializer.h"

#include <entt.hpp>
#include <glm/glm.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

    // Both the source and the reloaded scene fire camera hooks while this case
    // runs; drain the shared queue so later cases see it empty.
    void drainQueuedViewportEvents() {
        auto eventManager = Rubber::EventManager::create();
        eventManager->dispatchAllEvents();
        eventManager->dispatchAllEvents();
    }

} // namespace

TEST_CASE("serializer-roundtrips-inspector-backed-fields") {
    auto scene = Rubber::Scene::createWithoutSystems();

    Rubber::Entity entity = scene->createEntity("Serialized Entity");

    auto& transform = entity.addComponent<Rubber::TransformComponent>();
    transform.position = { 1.5f, -2.25f, 3.0f };
    transform.rotation = { 0.1f, 0.2f, 0.3f };
    transform.scale = { 2.0f, 3.0f, 4.0f };

    auto& sprite = entity.addComponent<Rubber::SpriteComponent>();
    sprite.color = { 0.12f, 0.34f, 0.56f, 0.78f };

    entity.addComponent<Rubber::VisibilityControlComponent>().isVisible = false;

    auto& camera = entity.addComponent<Rubber::CameraComponent>();
    camera.isPrimary = true;
    camera.isFixedAspectRatio = true;
    camera.camera.setProjectionType(Rubber::SceneCamera::ProjectionType::Persp);
    camera.camera.setPersFovY(0.9f);
    camera.camera.setPersNearClip(0.5f);
    camera.camera.setPersFarClip(500.0f);
    camera.camera.setOrthoSize(14.0f);
    camera.camera.setOrthoNearClip(-3.0f);
    camera.camera.setOrthoFarClip(7.0f);

    const std::filesystem::path file =
        std::filesystem::temp_directory_path() / "rb-serializer-roundtrip.rubber";
    {
        Rubber::SceneSerializer serializer(scene);
        serializer.serialize(file);
    }

    REQUIRE(std::filesystem::exists(file));
    REQUIRE(std::filesystem::file_size(file) > 0);

    // Freeze the identity placeholder the editor format writes today. It is
    // read back and discarded, not persistent identity.
    {
        std::ifstream stream(file);
        const std::string yaml((std::istreambuf_iterator<char>(stream)),
                               std::istreambuf_iterator<char>());
        REQUIRE(yaml.find("Entity: 123123141") != std::string::npos);
    }

    // Loading appends fresh entities into the receiving scene: one serialized
    // entity becomes exactly one new entity there.
    auto reloaded = Rubber::Scene::createWithoutSystems();
    bool loaded = false;
    {
        Rubber::SceneSerializer serializer(reloaded);
        loaded = serializer.deserialize(file);
    }
    REQUIRE(loaded);
    CHECK(reloaded->getRegistry().storage<entt::entity>().size() == 1);

    // The loaded entity is fresh, so it is located through the public
    // registry by the tag it carried in the file.
    Rubber::Entity loadedEntity;
    for (entt::entity handle : reloaded->getRegistry().storage<entt::entity>()) {
        Rubber::Entity candidate(handle, reloaded.get());
        if (candidate.getComponent<Rubber::TagComponent>().tag == "Serialized Entity")
            loadedEntity = candidate;
    }
    REQUIRE(loadedEntity);

    CHECK(loadedEntity.getComponent<Rubber::TagComponent>().tag == "Serialized Entity");

    const auto& loadedTransform = loadedEntity.getComponent<Rubber::TransformComponent>();
    CHECK(loadedTransform.position == glm::vec3(1.5f, -2.25f, 3.0f));
    CHECK(loadedTransform.rotation == glm::vec3(0.1f, 0.2f, 0.3f));
    CHECK(loadedTransform.scale == glm::vec3(2.0f, 3.0f, 4.0f));

    CHECK(loadedEntity.getComponent<Rubber::SpriteComponent>().color
          == glm::vec4(0.12f, 0.34f, 0.56f, 0.78f));

    CHECK(loadedEntity.getComponent<Rubber::VisibilityControlComponent>().isVisible == false);

    const auto& loadedCamera = loadedEntity.getComponent<Rubber::CameraComponent>();
    CHECK(loadedCamera.isPrimary == true);
    CHECK(loadedCamera.isFixedAspectRatio == true);
    CHECK(loadedCamera.camera.getProjectionType()
          == Rubber::SceneCamera::ProjectionType::Persp);
    CHECK(loadedCamera.camera.getPersFoVY() == 0.9f);
    CHECK(loadedCamera.camera.getPersNearClip() == 0.5f);
    CHECK(loadedCamera.camera.getPersFarClip() == 500.0f);
    CHECK(loadedCamera.camera.getOrthoSize() == 14.0f);
    CHECK(loadedCamera.camera.getOrthoNearClip() == -3.0f);
    CHECK(loadedCamera.camera.getOrthoFarClip() == 7.0f);

    drainQueuedViewportEvents();
    std::filesystem::remove(file);
}
