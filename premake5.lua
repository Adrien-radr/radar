-- Copy ext/rf/premake5.lua script as rf.lua to be able to include it
os.copyfile("ext/rf/premake5.lua", "ext/rf/rf.lua")

require "ext/rf"

workspace "Radar"
    language "C++"
    cppdialect "C++11"
    architecture "x86_64"

    platforms { "Windows", "Unix" }
    configurations { "Debug", "Release", "ReleaseDbg" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        optimize "On"

    filter "configurations:ReleaseDbg"
        optimize "Debug"
        buildoptions { "-fno-omit-frame-pointer" }

    filter {}

    buildoptions { "-W4" }

externalproject "rf"
    kind "StaticLib"    
    location "ext/rf"
    includedirs { "ext/rf/ext/glfw/include" }

externalproject "glfw3"
    kind "StaticLib"
    location "ext/rf"

project "sfmt"
    optimize "On"
    symbols "Off"

    kind "StaticLib"
    targetdir "ext/sfmt"
    includedirs { "ext/sfmt" }
    files { "ext/sfmt/*" }
    defines { "HAVE_SSE2=1", "SFMT_MEXP=19937" }

project "radar"
    kind "ConsoleApp"
    targetdir "bin/"
    debugdir "bin/"
    defines { "GLEW_STATIC" }
    defines { "HAVE_SSE2=1", "SFMT_MEXP=19937" }

    files { "src/**.cpp", "src/**.h" }
    removefiles { "src/radar_unix.cpp", "src/radar_win32.cpp" }
    includedirs { "src", "ext/rf/include", "ext/sfmt", "ext/rf/ext/cjson", 
                  "ext/openal-soft/include", "ext/rf/ext/glew/include", "ext/rf/ext/glfw/include" }

    libdirs { "ext/rf/lib", "ext/openal-soft/build" }

    filter "configurations:Debug"
        links { "rf_d", "glfw3_d" }

    filter "configurations:ReleaseDbg"
        links { "rf_p", "glfw3_p" }

    filter { "configurations:Release" }
        links { "rf", "glfw3" }

    filter "platforms:Windows"
        links { "openal32", "opengl32", "PowrProf" }

    filter "platforms:Unix"
        links { "openal", "GL", "X11", "dl", "pthread" }

    filter {}

    links { "sfmt"  }
