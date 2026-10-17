project "UnitTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    -- Explicit sources: no Engine link, PCH, graphics, windowing or editor code.
    files {
        "unit/UnitTests.cpp",
        "src/CameraInverseCheck.h", "src/CameraInverseCheck.cpp",
        "../Vendor/doctest/doctest.h"
    }
    includedirs {
        "../Vendor/doctest",
        "src",
        "../Engine/Source",
        "../Engine/RBVendor/glm"
    }
    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
        symbols "on"

    filter "configurations:Dist"
        runtime "Release"
        optimize "on"
        symbols "off"

    filter {}
