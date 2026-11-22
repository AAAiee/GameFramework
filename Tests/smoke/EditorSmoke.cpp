// Editor data-path smoke on a display/GL host: command-layer New/entity/
// Transform/Save/Load with an explicit path. This drives Scene plus
// SceneSerializer directly, the same bodies EditorLayer::newScene and the
// dialog-free saveSceneTo/openSceneFrom call, so no ImGui layer, native
// dialog, or editor panel is involved. The oracle is the process exit code.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include "Rubber/Core/Application.h"
#include "Rubber/Core/Log.h"
#include "Rubber/Scene/Scene.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Scene/Utili/Entity.h"
#include "Rubber/Scene/Utili/Serializer/SceneSerializer.h"

#include <filesystem>
#include <fstream>

int main() {
    Rubber::Logger::Init();

    Rubber::Application application("EditorSmoke");

    // New: fresh scene with its systems installed (EditorLayer::newScene body).
    auto scene = Rubber::Scene::create();

    // Entity with a Transform, as the Inspector would create them.
    Rubber::Entity entity = scene->createEntity("EditorSmoke Entity");
    auto& transform = entity.addComponent<Rubber::TransformComponent>();
    transform.position = { 1.0f, 2.0f, 3.0f };

    // Save: explicit path, no fileDialog (EditorLayer::saveSceneTo body).
    const std::filesystem::path file =
        std::filesystem::temp_directory_path() / "EditorSmoke.rubber";
    {
        Rubber::SceneSerializer serializer(scene);
        serializer.serialize(file);
    }

    // Load: deserialize into a fresh scene (EditorLayer::openSceneFrom body).
    auto loaded = Rubber::Scene::create();
    bool deserialized = false;
    {
        Rubber::SceneSerializer serializer(loaded);
        deserialized = serializer.deserialize(file);
    }

    std::ifstream stream(file);
    const bool nonempty = stream.good() && stream.peek() != std::char_traits<char>::eof();
    if (!nonempty || !deserialized) {
        return 1;
    }

    scene.reset();
    loaded.reset();
    return 0;
}
