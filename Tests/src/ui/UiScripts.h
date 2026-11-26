#pragma once

namespace Rubber {
    class Application;

    namespace UiScript {
        void checkHierarchy(Application& application);
        void checkInspector(Application& application);
        void checkFiles(Application& application);
        void checkViewPort(Application& application);
        void checkExit(Application& application);
        void checkEscape(Application& application);
    }
}
