#include "pch.h"
#include "UiScripts.h"
#include "UiScript.h"
#include "EditorLayer.h"
#include "Rubber/Scene/SceneCamera.h"

#include <iostream>

namespace Rubber::UiScript {
    namespace {
        Script inspector(Context& ctx) {
            co_await settle(ctx, 20);

            auto scene = ctx.editor.activeScene();
            Entity entity = scene->createEntity("Inspector Entity");
            co_await settle(ctx);

            expect("Inspector Entity");
            ctx.driver.click("Inspector Entity");
            co_await settle(ctx);
            expect("Add Component");
            expect("##Tag");
            ctx.driver.click("##Tag");
            co_await settle(ctx);

            ctx.driver.click("Add Component");
            co_await settle(ctx, 1);
            ctx.driver.click("Sprite Renderer");
            co_await settle(ctx);
            require(entity.hasComponent<SpriteComponent>(),
                "Add Component / Sprite Renderer did not add Sprite");
            std::cout << "OBSERVED: Add Component / Sprite Renderer reached the registry\n";

            expect("Color Picker");
            ctx.driver.click("Color Picker");
            co_await settle(ctx);
            expect("sv");
            expect("hue");
            std::cout << "OBSERVED: Color Picker opened the hue-bar popup\n";

            ctx.driver.click("ViewPort");
            co_await settle(ctx, 1);
            const auto beforeColor = entity.getComponent<SpriteComponent>().color;
            ctx.driver.drag("##X", ImVec2(-80.0f, 0.0f));
            co_await settle(ctx);
            const auto color = entity.getComponent<SpriteComponent>().color;
            require(color.r < beforeColor.r - 0.01f,
                "ColorEdit4 ##X drag did not edit Sprite color");
            std::cout << "OBSERVED: ColorEdit4 ##X drag edited Sprite color.r to "
                << color.r << "\n";

            ctx.driver.click("...", 0, 1);
            co_await settle(ctx, 1);
            ctx.driver.click("Remove Component");
            co_await settle(ctx);
            require(!entity.hasComponent<SpriteComponent>(),
                "Remove Component left Sprite attached");
            std::cout << "OBSERVED: Remove Component removed Sprite Renderer\n";

            ctx.driver.click("Add Component");
            co_await settle(ctx, 1);
            ctx.driver.click("Transform");
            co_await settle(ctx);
            require(entity.hasComponent<TransformComponent>(),
                "Add Component / Transform did not add Transform");
            std::cout << "OBSERVED: Add Component / Transform reached the registry\n";

            auto& transform = entity.getComponent<TransformComponent>();
            ctx.driver.drag("##X", ImVec2(80.0f, 0.0f));
            co_await settle(ctx);
            require(transform.position.x != 0.0f, "Transform X drag did not edit the component");
            ctx.driver.click("X");
            co_await settle(ctx, 1);
            require(transform.position.x == 0.0f, "Transform X reset did not run");

            const float beforeY = transform.position.y;
            ctx.driver.drag("##Y", ImVec2(80.0f, 0.0f));
            co_await settle(ctx);
            require(transform.position.y != beforeY, "Transform Y drag did not edit the component");
            ctx.driver.click("Y");
            co_await settle(ctx, 1);
            require(transform.position.y == 0.0f, "Transform Y reset did not run");

            ctx.driver.drag("##Z", ImVec2(80.0f, 0.0f));
            co_await settle(ctx);
            require(transform.position.z != 0.0f, "Transform Z drag did not edit the component");
            ctx.driver.click("Z");
            co_await settle(ctx, 1);
            require(transform.position.z == 0.0f, "Transform Z reset did not run");
            std::cout << "OBSERVED: Transform X, Y, Z reset buttons and drags edited the component\n";

            ctx.driver.click("Add Component");
            co_await settle(ctx, 1);
            ctx.driver.click("Camera");
            co_await settle(ctx);
            require(entity.hasComponent<CameraComponent>(),
                "Add Component / Camera did not add Camera");
            std::cout << "OBSERVED: Add Component / Camera reached the registry\n";
            expect("Primary");
            expect("IsFixedRatio");
            expect("Projection");
            expect("Size");

            auto& camera = entity.getComponent<CameraComponent>();
            const float beforeSize = camera.camera.getOrthoSize();
            ctx.driver.click("Primary");
            ctx.driver.click("IsFixedRatio");
            ctx.driver.drag("Size", ImVec2(80.0f, 0.0f));
            co_await settle(ctx);
            require(camera.isPrimary, "Primary checkbox did not toggle");
            require(camera.isFixedAspectRatio, "IsFixedRatio checkbox did not toggle");
            require(camera.camera.getOrthoSize() != beforeSize,
                "Size drag did not edit the camera");
            std::cout << "OBSERVED: Camera Primary, IsFixedRatio, and Size edited\n";

            ctx.driver.openCombo("Projection");
            co_await settle(ctx);
            expect("Orthographic");
            expect("Perspective");
            ctx.driver.click("Perspective");
            co_await settle(ctx);
            require(camera.camera.getProjectionType() == SceneCamera::ProjectionType::Persp,
                "Projection / Perspective did not switch the camera");
            std::cout << "OBSERVED: Projection switched to Perspective\n";

            expect("FOVY");
            expect("NearClip");
            expect("FarClip");
            const float beforeFov = camera.camera.getPersFoVY();
            const float beforeNear = camera.camera.getPersNearClip();
            const float beforeFar = camera.camera.getPersFarClip();
            ctx.driver.drag("FOVY", ImVec2(40.0f, 0.0f));
            ctx.driver.drag("NearClip", ImVec2(40.0f, 0.0f));
            ctx.driver.drag("FarClip", ImVec2(40.0f, 0.0f));
            co_await settle(ctx);
            require(camera.camera.getPersFoVY() != beforeFov, "FOVY drag did not edit the camera");
            require(camera.camera.getPersNearClip() != beforeNear,
                "NearClip drag did not edit the camera");
            require(camera.camera.getPersFarClip() != beforeFar,
                "FarClip drag did not edit the camera");
            std::cout << "OBSERVED: FOVY, NearClip, and FarClip drags edited the perspective camera\n";

            ctx.driver.openCombo("Projection");
            co_await settle(ctx);
            ctx.driver.click("Orthographic");
            co_await settle(ctx);
            require(camera.camera.getProjectionType() == SceneCamera::ProjectionType::Ortho,
                "Projection / Orthographic did not switch the camera");
            expect("Size");
            expect("NearClip");
            expect("FarClip");
            std::cout << "OBSERVED: Projection switched to Orthographic\n";

            ctx.driver.click("Add Component");
            co_await settle(ctx, 1);
            ctx.driver.click("Visibility");
            co_await settle(ctx);
            require(entity.hasComponent<VisibilityControlComponent>(),
                "Add Component / Visibility did not add Visibility");
            std::cout << "OBSERVED: Add Component / Visibility reached the registry\n";
            expect("isVisible");
            ctx.driver.click("isVisible");
            co_await settle(ctx);
            require(!entity.getComponent<VisibilityControlComponent>().isVisible,
                "isVisible checkbox did not toggle");
            std::cout << "OBSERVED: isVisible checkbox toggled off\n";
        }
    }

    void checkInspector(Application& application) {
        run(application, inspector);
    }
}
