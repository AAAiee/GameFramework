project "Engine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   staticruntime "on"

   files { "Source/**.h", "Source/**.cpp", "RBVendor/stb_image/stb_image.h" , "RBVendor/stb_image/stb_image.cpp"}
   
   -- Compile the existing string adapter once into Engine, shared by Editor and Regression.
   files { "RBVendor/imGui/misc/cpp/imgui_stdlib.cpp" }
   filter "files:RBVendor/imGui/misc/cpp/imgui_stdlib.cpp"
       flags { "NoPCH" }
   filter {}

   IncludeDir = {}
   IncludeDir["GLFW"]    = "RBVendor/glfw/include"
   IncludeDir["Glad"]    = "RBVendor/glad/include"
   IncludeDir["spdlogs"] ="RBVendor/spdlogs/include"
   IncludeDir["imGui"]   = "RBVendor/imGui"
   IncludeDir["stb_image"] = "RBVendor/stb_image"
   IncludeDir["glm"]       = "RBVendor/glm"
   IncludeDir["entt"]     = "RBVendor/entt/include"
   IncludeDir["yaml_cpp"] = "RBVendor/yaml-cpp/include"


   includedirs
   {
      "Source",
      "%{IncludeDir.spdlogs}",
      "%{IncludeDir.GLFW}",
      "%{IncludeDir.Glad}",
      "%{IncludeDir.imGui}",
      "%{IncludeDir.glm}",
      "%{IncludeDir.stb_image}",
      "%{IncludeDir.entt}",
      "%{IncludeDir.yaml_cpp}"
   } 

   links
   {
        "GLFW",
        "Glad",
        "imGui",
        "opengl32.lib",
        "yaml-cpp"
   }

   pchheader "pch.h"
   pchsource "Source/pch.cpp"

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")
 
   defines {"_CRT_SECURE_NO_WARNINGS","RUBBER_BUILD", "GLFW_INCLUDE_NONE"}

   filter "system:windows"
       systemversion "latest"
       defines { "RB_PLATFORM_WINDOWS" }

   filter "configurations:Debug"
       defines { "RB_DEBUG", "RB_ENABLE_ASSERTS" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"