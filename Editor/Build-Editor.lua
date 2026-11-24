project "Editor"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   staticruntime "on"

   files { "src/**.h", "src/**.cpp" }

   includedirs
   {
      "src",

	  -- Include Core
	  "../Engine/Source",
      "../Engine/RBVendor/spdlogs/include",
      "../Engine/RBVendor/glm",
      "../Engine/RBVendor/imGui",
      "../Engine/RBVendor/entt/include"

   }

   links
   {
      "Engine"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   postbuildcommands
   {
       "{COPY} \"assets\" \"%{cfg.targetdir}/assets\"",
       "{COPY} \"imgui.ini\" \"%{cfg.targetdir}/\""
   }

   defines { "RB_BUILD_EDITOR", "IMGUI_ENABLE_TEST_ENGINE" }
   
   filter "system:windows"
       systemversion "latest"
       defines { "RB_PLATFORM_WINDOWS" }

   filter "configurations:Debug"
       defines { "RB_DEBUG", "RB_ENABLE_ASSERTS"}
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
       optimize "speed"
       symbols "off"


