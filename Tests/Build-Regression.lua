project "Regression"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    files { "src/**.h", "src/**.cpp" }
    includedirs {
        "../Engine/Source",
        "../Engine/RBVendor/spdlogs/include",
        "../Engine/RBVendor/glm",
        "../Engine/RBVendor/entt/include",
        "../Engine/RBVendor/glfw/include"
    }
    links { "Engine" }
    defines { "GLFW_INCLUDE_NONE" }
    targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
    objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")
    postbuildcommands {
        "{COPY} \"../Editor/assets\" \"%{cfg.targetdir}/assets\""
    }

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
