// Keep doctest's implementation/main in this TU, outside Regression's src glob.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "CameraInverseCheck.h"

#ifdef DOCTEST_CONFIG_DISABLE
#error UnitTests requires active doctest assertions in every configuration
#endif

TEST_CASE("camera-inverse") {
    CHECK_NOTHROW(Rubber::checkCameraInverse());
}
