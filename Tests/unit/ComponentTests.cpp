// TransformComponent's matrix is the engine's transform contract: translation
// after rotation after scale. Default state must be the identity; a translation
// must move the origin and leave the linear part untouched.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include <doctest.h>

#include "Rubber/Scene/Utili/Component.h"

#include <glm/glm.hpp>

namespace {

    bool sameElements(const glm::mat4& lhs, const glm::mat4& rhs) {
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                if (lhs[column][row] != rhs[column][row])
                    return false;
        return true;
    }

} // namespace

TEST_CASE("transform-default-state-is-identity") {
    Rubber::TransformComponent transform;
    CHECK(sameElements(transform.getTransformMatrix(), glm::mat4(1.0f)));
}

TEST_CASE("transform-translation-moves-the-origin") {
    Rubber::TransformComponent transform;
    transform.position = { 2.0f, 3.0f, 4.0f };
    const glm::mat4 matrix = transform.getTransformMatrix();

    const glm::vec4 origin = matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    CHECK(origin == glm::vec4(2.0f, 3.0f, 4.0f, 1.0f));

    // The linear part stays the identity, so axis points translate verbatim.
    const glm::vec4 xAxisPoint = matrix * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    CHECK(xAxisPoint == glm::vec4(3.0f, 3.0f, 4.0f, 1.0f));
}
