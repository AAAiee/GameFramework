// Engine smoke on a display/GL host: real Application, real Scene with systems
// installed, a few fixed updates. The oracle is the process exit code; no
// matrix or render output is asserted.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include "Rubber/Core/Application.h"
#include "Rubber/Core/Log.h"
#include "Rubber/Scene/Scene.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Scene/Utili/Entity.h"

int main() {
    Rubber::Logger::Init();

    Rubber::Application application("EngineSmoke");

    auto scene = Rubber::Scene::create();
    Rubber::Entity entity = scene->createEntity("Smoke Entity");
    entity.addComponent<Rubber::TransformComponent>();

    // Fixed-update cadence without running the full editor loop.
    for (int step = 0; step < 4; ++step) {
        scene->onSceneUpdate(1.0f / 120.0f);
    }

    // The scene must die while the Application facilities are still alive.
    scene.reset();
    return 0;
}
