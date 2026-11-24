#include "pch.h"
#include "UiScripts.h"
#include "UiScript.h"
#include "EditorLayer.h"
#include "Rubber/Input/KeyCodes.h"

#include <iostream>

namespace Rubber::UiScript {
    namespace {
        Script files(Context& ctx) {
            co_await settle(ctx, 20);

            ctx.editor.activeScene()->createEntity("Files Entity");
            co_await settle(ctx);
            ctx.driver.click("File");
            co_await settle(ctx, 1);
            ctx.driver.click("New");
            co_await settle(ctx);
            require(taggedCount(*ctx.editor.activeScene()) == 0,
                "File / New did not clear the scene");
            std::cout << "OBSERVED: File / New cleared the scene\n";

            ctx.editor.activeScene()->createEntity("Files Entity");
            co_await settle(ctx);
            ctx.driver.chord(RB_KEY_N);
            co_await settle(ctx);
            require(taggedCount(*ctx.editor.activeScene()) == 0,
                "Ctrl+N did not clear the scene");
            std::cout << "OBSERVED: Ctrl+N overlay cleared the scene through shortcutCallBack\n";

            ctx.driver.click("File");
            co_await settle(ctx, 1);
            ctx.dialogSeen = false;
            ctx.driver.beginDialogCancel();
            ctx.driver.click("Open...");
            co_await settle(ctx);
            require(ctx.dialogSeen, "File / Open... did not open a native dialog");
            std::cout << "OBSERVED: File / Open... opened a native dialog\n";

            ctx.driver.click("File");
            co_await settle(ctx, 1);
            ctx.dialogSeen = false;
            ctx.driver.beginDialogCancel();
            ctx.driver.click("Save As...");
            co_await settle(ctx);
            require(ctx.dialogSeen, "File / Save As... did not open a native dialog");
            std::cout << "OBSERVED: File / Save As... opened a native dialog\n";

            ctx.dialogSeen = false;
            ctx.driver.beginDialogCancel();
            ctx.driver.chord(RB_KEY_O);
            co_await settle(ctx);
            require(ctx.dialogSeen, "Ctrl+O overlay did not open a native dialog");
            std::cout << "OBSERVED: Ctrl+O overlay opened a native dialog\n";

            ctx.dialogSeen = false;
            ctx.driver.beginDialogCancel();
            ctx.driver.chord(RB_KEY_S);
            co_await settle(ctx);
            require(ctx.dialogSeen, "Ctrl+S overlay did not open a native dialog");
            std::cout << "OBSERVED: Ctrl+S overlay opened a native dialog\n";
        }
    }

    void checkFiles(Application& application) {
        run(application, files);
    }
}
