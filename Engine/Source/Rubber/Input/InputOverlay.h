#pragma once

namespace Rubber {

    /** Synthetic key and mouse state for Regression.
     * WindowInput consults keys before GLFW. ImGuiLayer applies mouse after the
     * GLFW backend NewFrame so injected events win over the real cursor.
     * Production never sets this, so glfwGetKey remains the only source.
     */
    class InputOverlay {
    public:
        static void press(int keyCode);
        static void release(int keyCode);
        static void clear();
        static bool isPressed(int keyCode);
        static void setMousePos(float x, float y);
        static void setMouseButton(int button, bool down);
        static void applyToImGui();
    };

}
