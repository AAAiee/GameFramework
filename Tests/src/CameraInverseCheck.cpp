#include "CameraInverseCheck.h"
#include "Rubber/Renderer/Camera.h"
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Rubber {
    static_assert(std::is_same_v<
        decltype(std::declval<const Camera&>().getInverseProjection()), glm::mat4>,
        "Camera inverse projection must return an owned glm::mat4 value");

    void checkCameraInverse() {
        const glm::mat4 projection = glm::perspective(
            glm::radians(57.0f), 16.0f / 9.0f, 0.25f, 150.0f);
        Camera camera;
        camera.setProjection(projection);
        const Camera& view = camera;

        // Binding the returned value extends its lifetime in the caller. A
        // reference returned to a destroyed callee temporary cannot do this.
        const glm::mat4& inverse = view.getInverseProjection();
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                if (view.getProjection()[column][row] != projection[column][row]) {
                    throw std::runtime_error("Inverse accessor changed the original projection");
                }
            }
        }

        // The snapshot must also survive subsequent changes to camera storage.
        camera.setProjection(glm::mat4(1.0f));
        const glm::mat4 product = projection * inverse;
        constexpr float tolerance = 1.0e-5f; // Absolute tolerance per element.
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                const float expected = column == row ? 1.0f : 0.0f;
                // This form rejects NaN as well as excessive finite error.
                if (!(std::abs(product[column][row] - expected) <= tolerance)) {
                    throw std::runtime_error("Projection times retained inverse is not identity within 1e-5");
                }
            }
        }
    }
}
