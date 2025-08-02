-- premake5.lua

function setup_dirs(path)
   zip.extract("assets/assets.zip", path)
   os.mkdir("lib")
end


workspace "Ray-shooter"
   architecture "x64"
   configurations { "Debug", "Release" }
   location ""	

project "Ray-shooter"
   language "C"
   cdialect "C11"
   compileas "C"
   targetdir "bin/%{cfg.buildcfg}"
   location ""			
	includedirs { "thirdparty" }
	includedirs { "src" }
	links{ "glfw3"}
	libdirs { "lib" }

   files { "**.h", "**.c"}

   filter "configurations:Debug"
      kind "ConsoleApp"
      defines { "DEBUG", "_CRT_SECURE_NO_WARNINGS" }
      symbols "On"
      postbuildcommands 
      { 
	  setup_dirs("bin/Debug/assets")
      }

   filter "configurations:Release"
      kind "ConsoleApp"
      defines { "NDEBUG", "_CRT_SECURE_NO_WARNINGS" }
      optimize "On"
      postbuildcommands 
      { 
      	 setup_dirs("bin/Release/assets")
      }	
      