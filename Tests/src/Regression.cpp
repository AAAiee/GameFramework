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
#include "RendererBatchesCheck.h"
#include "EditorIssue15Fixture.h"
#include "EntityTagsCheck.h"
#include "ViewportPixelsCheck.h"
#include "MinimizeRestoreCheck.h"
#include "LayerPartitionCheck.h"
#include "EventChecks.h"
#include <windows.h>

namespace {
    // Never switch the user's input desktop. All regression windows, including
    // modal native dialogs, stay on this process's private desktop.
    class RegressionDesktop {
    public:
        RegressionDesktop() : m_Previous(GetThreadDesktop(GetCurrentThreadId())) {
            const auto name = L"RubberRegression-" + std::to_wstring(GetCurrentProcessId());
            m_Desktop = CreateDesktopW(name.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
            if (!m_Desktop || !SetThreadDesktop(m_Desktop)) {
                if (m_Desktop) CloseDesktop(m_Desktop);
                throw std::runtime_error("Cannot isolate regression UI; refusing to open dialogs on the user's desktop");
            }
        }
        ~RegressionDesktop() {
            SetThreadDesktop(m_Previous);
            CloseDesktop(m_Desktop);
        }
        RegressionDesktop(const RegressionDesktop&) = delete;
        RegressionDesktop& operator=(const RegressionDesktop&) = delete;
    private:
        HDESK m_Previous;
        HDESK m_Desktop = nullptr;
    };

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
    if (argc > 2 || (argc == 2 && (std::string_view(argv[1]) != "scene-hooks" && std::string_view(argv[1]) != "editor-issue-12" && std::string_view(argv[1]) != "camera-inverse" && std::string_view(argv[1]) != "renderer-batches" && std::string_view(argv[1]) != "editor-issue-15" && std::string_view(argv[1]) != "entity-tags" && std::string_view(argv[1]) != "editor-issue-16" && std::string_view(argv[1]) != "viewport-pixels" && std::string_view(argv[1]) != "editor-issue-17" && std::string_view(argv[1]) != "minimize-restore" && std::string_view(argv[1]) != "editor-issue-18" && std::string_view(argv[1]) != "layer-partition" && std::string_view(argv[1]) != "typed-events" && std::string_view(argv[1]) != "event-names"))) {
        std::cerr << "Usage: Regression.exe [scene-hooks|editor-issue-12|camera-inverse|renderer-batches|editor-issue-15|entity-tags|editor-issue-16|viewport-pixels|editor-issue-17|minimize-restore|editor-issue-18|layer-partition|typed-events|event-names]\n";
        return 2;
    }
    try {
        RegressionDesktop desktop;
        if (argc == 1 || std::string_view(argv[1]) == "camera-inverse") {
            Rubber::checkCameraInverse();
            std::cout << "PASS: camera-inverse\n";
            if (argc == 2) {
                return 0;
            }
        }
        if (argc == 1 || std::string_view(argv[1]) == "layer-partition") {
            Rubber::checkLayerPartition();
            std::cout << "PASS: layer-partition\n";
            if (argc == 2) {
                return 0;
            }
        }
        Rubber::Logger::Init();
        if (argc == 1 || std::string_view(argv[1]) == "typed-events") {
            Rubber::checkTypedEvents();
            std::cout << "PASS: typed-events\n";
            if (argc == 2) return 0;
        }
        if (argc == 1 || std::string_view(argv[1]) == "event-names") {
            Rubber::checkEventNames();
            std::cout << "PASS: event-names\n";
            if (argc == 2) return 0;
        }
        require(glfwInit() == GLFW_TRUE, "GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        Rubber::Application application("Rubber regression");
        glfwDefaultWindowHints();
        glfwHideWindow(static_cast<GLFWwindow*>(Rubber::Application::getWindow().getNativeWindow()));
        if (argc == 1 || std::string_view(argv[1]) == "scene-hooks") {
            checkSceneHooks();
            std::cout << "PASS: scene-hooks\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "renderer-batches") {
            Rubber::checkRendererBatches();
            std::cout << "PASS: renderer-batches\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "entity-tags" || std::string_view(argv[1]) == "editor-issue-16") {
            Rubber::checkEntityTags();
            std::cout << "PASS: entity-tags\n";
            if (argc == 1 || std::string_view(argv[1]) == "editor-issue-16")
                std::cout << "PASS: editor-issue-16\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "editor-issue-12") {
            Rubber::checkEditorIssue12(application);
            std::cout << "PASS: editor-issue-12\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "editor-issue-15") {
            Rubber::checkEditorIssue15();
            std::cout << "PASS: editor-issue-15\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "viewport-pixels" || std::string_view(argv[1]) == "editor-issue-17") {
            Rubber::checkViewportPixels();
            std::cout << "PASS: viewport-pixels\n";
            if (argc == 1 || std::string_view(argv[1]) == "editor-issue-17")
                std::cout << "PASS: editor-issue-17\n";
        }
        if (argc == 1 || std::string_view(argv[1]) == "minimize-restore" || std::string_view(argv[1]) == "editor-issue-18") {
            Rubber::checkMinimizeRestore(application);
            std::cout << "PASS: minimize-restore\n";
            if (argc == 1 || std::string_view(argv[1]) == "editor-issue-18")
                std::cout << "PASS: editor-issue-18\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
