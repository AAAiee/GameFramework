#include "pch.h"
#include "Rubber/Core/Application.h"
#include "Rubber/Event/EventManager.h"
#include "Rubber/Event/AppEvent.h"
#include <cmath>
#include "Rubber/Scene/Scene.h"
#include "Rubber/Scene/Utili/Entity.h"
#include "Rubber/Scene/Utili/Component.h"
#include "Rubber/Window/Window.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include "EditorIssue12Fixture.h"
#include "CameraInverseCheck.h"

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
        scene->updatesViewportSize({ 640.0f, 480.0f });
        auto& camera = entity.addComponent<Rubber::CameraComponent>();
        Rubber::Application::getEventManager()->dispatchAllEvents();
        Rubber::Application::getEventManager()->dispatchAllEvents();
        const auto& projection = camera.camera.getProjection();
        require(std::abs(projection[1][1] / projection[0][0] - 640.0f / 480.0f) < 0.001f,
            "Camera construction hook did not deliver the stored viewport");
        entity.removeComponent<Rubber::CameraComponent>();
        require(!entity.hasComponent<Rubber::CameraComponent>(), "Camera removal did not reach the registry");
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
    if (argc > 2 || (argc == 2 && (std::string_view(argv[1]) != "scene-hooks" && std::string_view(argv[1]) != "editor-issue-12" && std::string_view(argv[1]) != "camera-inverse"))) {
        std::cerr << "Usage: Regression.exe [scene-hooks|editor-issue-12|camera-inverse]\n";
        return 2;
    }
    try {
        if (argc == 1 || std::string_view(argv[1]) == "camera-inverse") {
            Rubber::checkCameraInverse();
            std::cout << "PASS: camera-inverse\n";
            if (argc == 2) {
                return 0;
            }
        }
        Rubber::Logger::Init();
        Rubber::Application application("Rubber regression");
        glfwHideWindow(static_cast<GLFWwindow*>(Rubber::Application::getWindow().getNativeWindow()));
        if (argc == 1 || std::string_view(argv[1]) == "scene-hooks") {
            checkSceneHooks();
            std::cout << "PASS: scene-hooks\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "editor-issue-12") {
            Rubber::checkEditorIssue12(application);
            std::cout << "PASS: editor-issue-12\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
