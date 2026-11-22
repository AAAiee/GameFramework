// SceneCamera behavior through its public setters and the inherited projection.
// Expected values are derived from the projection contracts (frustum edges map
// to NDC corners), not from the implementation's formulas.
#include <pch.h> // Engine headers rely on the Engine's common include surface.
#include <doctest.h>

#include "Rubber/Scene/SceneCamera.h"

#include <glm/glm.hpp>

#include <cmath>

namespace {

    // Divides by w so both orthographic and perspective matrices land in NDC.
    // All points used below are in front of the camera, so w stays positive.
    glm::vec3 projected(const glm::mat4& projection, const glm::vec3& point) {
        const glm::vec4 clip = projection * glm::vec4(point, 1.0f);
        return glm::vec3(clip) / clip.w;
    }

    // 0.1 percent tolerance: float rounding is far below this, while a wrong
    // projection (for example an identity matrix) misses by whole units.
    doctest::Approx closeTo(float expected) {
        return doctest::Approx(expected).epsilon(0.001f);
    }

    void checkAllFinite(const glm::mat4& matrix) {
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                CHECK(std::isfinite(matrix[column][row]));
    }

} // namespace

TEST_CASE("scene-camera-default-is-orthographic") {
    Rubber::SceneCamera camera;
    CHECK(camera.getProjectionType() == Rubber::SceneCamera::ProjectionType::Ortho);
    checkAllFinite(camera.getProjection());

    // Defaults: size 10 (vertical), aspect 16/9, near -1, far 1.
    // The right/top edge at the near plane maps to the (+1,+1,-1) NDC corner.
    const float halfWidth = 10.0f * (16.0f / 9.0f) / 2.0f;
    glm::vec3 corner = projected(camera.getProjection(), { halfWidth, 5.0f, 1.0f });
    CHECK(corner.x == closeTo(1.0f));
    CHECK(corner.y == closeTo(1.0f));
    CHECK(corner.z == closeTo(-1.0f));

    // The left/bottom edge at the far plane maps to the (-1,-1,+1) corner.
    glm::vec3 farCorner = projected(camera.getProjection(), { -halfWidth, -5.0f, -1.0f });
    CHECK(farCorner.x == closeTo(-1.0f));
    CHECK(farCorner.y == closeTo(-1.0f));
    CHECK(farCorner.z == closeTo(1.0f));
}

TEST_CASE("scene-camera-setOrtho-uses-size-aspect-and-clips") {
    Rubber::SceneCamera camera;
    camera.setOrtho(20.0f, 2.0f, -10.0f, 100.0f);
    checkAllFinite(camera.getProjection());

    // Half extents: width = size * aspect = 40 wide, height = size = 20 tall.
    // Eye z = -near maps to NDC z = -1, eye z = -far maps to NDC z = +1.
    glm::vec3 nearCorner = projected(camera.getProjection(), { 20.0f, 10.0f, 10.0f });
    CHECK(nearCorner.x == closeTo(1.0f));
    CHECK(nearCorner.y == closeTo(1.0f));
    CHECK(nearCorner.z == closeTo(-1.0f));

    glm::vec3 farCorner = projected(camera.getProjection(), { -20.0f, -10.0f, -100.0f });
    CHECK(farCorner.x == closeTo(-1.0f));
    CHECK(farCorner.y == closeTo(-1.0f));
    CHECK(farCorner.z == closeTo(1.0f));
}

TEST_CASE("scene-camera-onResize-recomputes-aspect-only") {
    Rubber::SceneCamera camera;
    camera.setOrtho(20.0f, 2.0f, -10.0f, 100.0f);
    camera.onResize(800.0f, 600.0f);

    // Aspect is now 4/3; the vertical half extent (size/2 = 10) is unchanged
    // and the horizontal half extent follows the new aspect (20 * 4/3 / 2).
    const float halfWidth = 20.0f * (4.0f / 3.0f) / 2.0f;
    glm::vec3 corner = projected(camera.getProjection(), { halfWidth, 10.0f, 10.0f });
    CHECK(corner.x == closeTo(1.0f));
    CHECK(corner.y == closeTo(1.0f));
    CHECK(corner.z == closeTo(-1.0f));
}

TEST_CASE("scene-camera-perspective-projection") {
    Rubber::SceneCamera camera;
    camera.setProjectionType(Rubber::SceneCamera::ProjectionType::Persp);
    CHECK(camera.getProjectionType() == Rubber::SceneCamera::ProjectionType::Persp);
    checkAllFinite(camera.getProjection());

    // Defaults: fov 45 degrees, near 0.01, far 10000, aspect 16/9.
    // The near and far plane centers pin the depth range to the full NDC z.
    glm::vec3 nearCenter = projected(camera.getProjection(), { 0.0f, 0.0f, -0.01f });
    CHECK(nearCenter.x == closeTo(0.0f));
    CHECK(nearCenter.y == closeTo(0.0f));
    CHECK(nearCenter.z == closeTo(-1.0f));

    glm::vec3 farCenter = projected(camera.getProjection(), { 0.0f, 0.0f, -10000.0f });
    CHECK(farCenter.x == closeTo(0.0f));
    CHECK(farCenter.y == closeTo(0.0f));
    CHECK(farCenter.z == closeTo(1.0f));

    // The top edge of the near plane sits at tan(fov/2) * near eye height.
    const float topEdge = std::tan(glm::radians(45.0f) / 2.0f) * 0.01f;
    glm::vec3 top = projected(camera.getProjection(), { 0.0f, topEdge, -0.01f });
    CHECK(top.x == closeTo(0.0f));
    CHECK(top.y == closeTo(1.0f));
    CHECK(top.z == closeTo(-1.0f));
}
