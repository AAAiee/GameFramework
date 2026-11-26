#include "pch.h"
#include "UiScripts.h"
#include "UiScript.h"
#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

namespace Rubber::UiScript {
    namespace {
        Script viewPort(Context& ctx) {
            co_await settle(ctx, 20);
            expect("ViewPort");
            const glm::vec2 before = ctx.editor.viewPortDimension();
            ctx.driver.drag("##Splitter", ImVec2(80.0f, 0.0f), 1);
            co_await settle(ctx, 3);
            const glm::vec2 after = ctx.editor.viewPortDimension();
            require(after.x != before.x || after.y != before.y,
                "ViewPort splitter drag did not change panel size");
            std::cout << "OBSERVED: ViewPort splitter drag changed panel size to "
                << after.x << "x" << after.y << "\n";

            ImGuiWindow* setting = ImGui::FindWindowByName("Setting");
            require(setting && setting->Active && setting->DrawList
                && setting->DrawList->VtxBuffer.Size > 0,
                "Setting window did not submit stats");
            std::cout << "OBSERVED: Setting window submitted DrawCount stats\n";
        }
    }

    void checkViewPort(Application& application) {
        run(application, viewPort);
    }
}
