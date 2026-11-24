#pragma once

namespace Rubber {
    class Application;

    /** Clicks every catalogued Editor widget through ItemProbe. Throws unless
     * each production menu, inspector control, shortcut, splitter, and Exit
     * path is taken.
     */
    void checkEditorUi(Application& application);

    /** Posts Escape as a KeyPressedEvent and requires Application to close. */
    void checkEditorUiEscape(Application& application);
}
