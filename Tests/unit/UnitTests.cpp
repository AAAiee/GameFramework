// Keep doctest's implementation in this TU, outside Regression's src glob.
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include "CameraInverseCheck.h"
#include "Rubber/Core/Log.h"

#ifdef DOCTEST_CONFIG_DISABLE
#error UnitTests requires active doctest assertions in every configuration
#endif

TEST_CASE("camera-inverse") {
    CHECK_NOTHROW(Rubber::checkCameraInverse());
}

int main(int argc, char** argv) {
    // The Engine loggers must exist before any case constructs a Scene,
    // subscribes a handler, or lets an assert log. spdlog rejects duplicate
    // logger names, so initialization happens exactly once, here.
    Rubber::Logger::Init();
    doctest::Context context(argc, argv);
    return context.run();
}
