#include "pch.h"
#include "UiScripts.h"
#include "UiScript.h"
#include "EditorLayer.h"
#include "Rubber/imGui/ItemProbe.h"

#include <iostream>

namespace Rubber::UiScript {
    namespace {
        Script hierarchy(Context& ctx) {
            co_await settle(ctx, 20);
            std::cout << "OBSERVED: ui-catalog-dump\n";
            ItemProbe::dump(std::cout);
            expect("File");
            expect("Scene Hierarchy");
            expect("Property Channel");
            expect("ViewPort");
            expect("Setting");
            expect("##Splitter");

            ctx.driver.rightClickBlank("Scene Hierarchy");
            co_await settle(ctx);
            ctx.driver.click("Create Entity");
            co_await settle(ctx);

            auto scene = ctx.editor.activeScene();
            require(taggedCount(*scene) == 1, "Create Entity did not add a row");
            require(firstTagged(*scene).getComponent<TagComponent>().tag == "Empty Entity",
                "Create Entity did not name the entity Empty Entity");
            std::cout << "OBSERVED: Create Entity menu added Empty Entity\n";

            ctx.driver.click("Empty Entity");
            co_await settle(ctx);
            expect("Add Component");

            ctx.driver.click("Empty Entity", 1);
            co_await settle(ctx);
            ctx.driver.click("Delete Entity");
            co_await settle(ctx);

            require(taggedCount(*scene) == 0, "Delete Entity did not remove the row");
            std::cout << "OBSERVED: Delete Entity removed Empty Entity\n";
        }
    }

    void checkHierarchy(Application& application) {
        run(application, hierarchy);
    }
}
