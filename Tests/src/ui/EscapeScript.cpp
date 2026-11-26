#include "pch.h"
#include "UiScripts.h"
#include "UiScript.h"
#include "EditorLayer.h"
#include "Rubber/Core/Application.h"
#include "Rubber/Event/EventManager.h"
#include "Rubber/Event/KeyEvent.h"
#include "Rubber/Input/KeyCodes.h"

#include <iostream>

namespace Rubber::UiScript {
    namespace {
        Script escapeEditor(Context& ctx) {
            co_await settle(ctx, 12);
            Application::getEventManager()->enqueue(KeyPressedEvent(RB_KEY_ESCAPE, 0));
            ctx.closing = true;
        }
    }

    void checkEscape(Application& application) {
        run(application, escapeEditor);
        std::cout << "OBSERVED: Escape posted WindowCloseEvent and closed the editor\n";
    }
}
