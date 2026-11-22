project "UnitTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    -- Links the Engine static library so Scene/Entity/Serializer/LayerStack/EventManager
    -- symbols resolve. Headless cases never construct Application, call glfwInit,
    -- or create a window; windowing and GL code stays unreferenced. No editor
    -- sources, PCH or editor assets are used.
    files {
        "unit/UnitTests.cpp",
        "unit/SceneCameraTests.cpp",
        "unit/LayerStackTests.cpp",
        "unit/EventManagerTests.cpp",
        "unit/ComponentTests.cpp",
        "unit/SceneEntityTests.cpp",
        "unit/SceneSerializerTests.cpp",
        "src/CameraInverseCheck.h", "src/CameraInverseCheck.cpp",
        "../Vendor/doctest/doctest.h"
    }
    includedirs {
        "../Vendor/doctest",
        "src",
        "../Engine/Source",
        "../Engine/RBVendor/spdlogs/include",
        "../Engine/RBVendor/glm",
        "../Engine/RBVendor/entt/include",
        "../Engine/RBVendor/glfw/include",
        "../Engine/RBVendor/glad/include",
        "../Engine/RBVendor/yaml-cpp/include"
    }
    links { "Engine" }
    defines { "GLFW_INCLUDE_NONE" }
    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    filter "system:windows"
        systemversion "latest"
        defines { "RB_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines { "RB_DEBUG", "RB_ENABLE_ASSERTS" }
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        optimize "on"
        symbols "on"

    filter "configurations:Dist"
        defines { "DIST" }
        runtime "Release"
        optimize "on"
        symbols "off"

    filter {}
