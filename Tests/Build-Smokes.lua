-- Console smokes for a display/GL host. Both construct the real Application,
-- so hosted windows-latest may compile them but must not run them (no WGL).
-- Editor assets are copied because Renderer2D::init loads the engine's shader.
local function smokeProject(name, source)
    project (name)
        kind "ConsoleApp"
        language "C++"
        cppdialect "C++20"
        staticruntime "on"

        files { source }
        includedirs {
            "../Engine/Source",
            "../Engine/RBVendor/spdlogs/include",
            "../Engine/RBVendor/glm",
            "../Engine/RBVendor/entt/include",
            "../Engine/RBVendor/glfw/include",
            "../Engine/RBVendor/glad/include",
            "../Engine/RBVendor/yaml-cpp/include"
        }
        links { "Engine" }
        defines { "GLFW_INCLUDE_NONE", "IMGUI_ENABLE_TEST_ENGINE" }
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

        filter {}
end

smokeProject("EngineSmoke", "smoke/EngineSmoke.cpp")
smokeProject("EditorSmoke", "smoke/EditorSmoke.cpp")
