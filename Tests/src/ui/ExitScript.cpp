#include "pch.h"
#include "UiScripts.h"
#include "UiScript.h"
#include "EditorLayer.h"

#include <iostream>

namespace Rubber::UiScript {
    namespace {
        Script exitEditor(Context& ctx) {
            co_await settle(ctx, 20);
            ctx.driver.click("File");
            co_await settle(ctx, 1);
            ctx.driver.click("Exit");
            ctx.closing = true;
        }
    }

    void checkExit(Application& application) {
        run(application, exitEditor);
        std::cout << "OBSERVED: File / Exit closed the editor\n";
    }
}
