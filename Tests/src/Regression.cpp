#include "pch.h"
#include "Rubber/Core/Application.h"
#include "Rubber/Event/EventManager.h"
#include "Rubber/Scene/Scene.h"
#include "Rubber/Scene/Utili/Entity.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Window/Window.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    template<typename T, typename... Args>
    void checkComponent(Rubber::Entity entity, Args&&... args) {
        require(!entity.hasComponent<T>(), "Component unexpectedly present before addition");
        T& component = entity.addComponent<T>(std::forward<Args>(args)...);
        require(entity.hasComponent<T>(), "Component addition did not reach the registry");
        require(&component == &entity.getComponent<T>(), "Addition returned a different component");
        entity.removeComponent<T>();
        require(!entity.hasComponent<T>(), "Component removal did not reach the registry");
    }

    void checkSceneHooks() {
        auto scene = Rubber::Scene::create();
        auto entity = scene->createEntity("F01 hooks");
        require(entity.getComponent<Rubber::TagComponent>().tag == "F01 hooks",
            "Entity creation did not retain its tag");
        entity.removeComponent<Rubber::TagComponent>();
        checkComponent<Rubber::TagComponent>(entity, "replacement");
        checkComponent<Rubber::TransformComponent>(entity);
        checkComponent<Rubber::SpriteComponent>(entity);
        checkComponent<Rubber::CameraComponent>(entity);
        checkComponent<Rubber::VisibilityControlComponent>(entity);
        Rubber::Application::getEventManager()->dispatchAllEvents();
        Rubber::Application::getEventManager()->dispatchAllEvents();
        auto handle = static_cast<entt::entity>(entity);
        scene->removeEntity(entity);
        require(!scene->getRegistry().valid(handle), "Entity removal left a live registry handle");
    }
}

int main(int argc, char** argv) {
    if (argc > 2 || (argc == 2 && std::string_view(argv[1]) != "scene-hooks")) {
        std::cerr << "Usage: Regression.exe [scene-hooks]\n";
        return 2;
    }
    try {
        Rubber::Logger::Init();
        Rubber::Application application("Rubber regression");
        glfwHideWindow(static_cast<GLFWwindow*>(Rubber::Application::getWindow().getNativeWindow()));
        checkSceneHooks();
        std::cout << "PASS: scene-hooks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
