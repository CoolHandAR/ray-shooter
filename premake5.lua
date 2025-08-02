-- premake5.lua
workspace "Ray-shooter"
   architecture "x64"
   configurations { "Debug", "Release" }
   location "build/Ray-shooter"	

project "Ray-shooter"
   language "C"
   cdialect "C11"
   compileas "C"
   targetdir "bin/%{cfg.buildcfg}"
   location "build/Ray-shooter"			
	includedirs { "thirdparty" }
	includedirs { "src" }
	links{ "glfw3"}
	libdirs { "lib" }

   files { "**.h", "**.c"}

   filter "configurations:Debug"
      kind "ConsoleApp"
      defines { "DEBUG" }
      symbols "On"
      postbuildcommands 
      { 
      	 
      }

   filter "configurations:Release"
      kind "ConsoleApp"
      defines { "NDEBUG" }
      optimize "On"
      postbuildcommands 
      { 
      	 
      }	
      