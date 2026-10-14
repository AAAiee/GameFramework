#pragma once

namespace Rubber {
    class Application;

    /** Runs the real editor layer/window loop; throws unless scene editing and rendering pass. */
    void checkEditorIssue12(Application& application);
}
