-- Copy ext/rf/premake5.lua script as rf.lua to be able to include it
os.copyfile("ext/rf/premake5.lua", "ext/rf/rf.lua")

require "ext/rf"

workspace "Radar"
    language "C++"
    cppdialect "C++11"
    architecture "x86_64"

    configurations { "Debug", "Release" }
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        optimize "On"

project "sfmt"
    configurations { "Release" }
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
    defines { "GLEW_STATIC" }
    defines { "HAVE_SSE2=1", "SFMT_MEXP=19937" }

    files { "src/**.cpp", "src/**.h" }
    removefiles { "src/radar_unix.cpp", "src/radar_win32.cpp" }
    includedirs { "src", "ext/rf/include", "ext/sfmt", "ext/rf/ext/cjson", "ext/openal-soft/include" }

    libdirs { "ext/rf/lib", "ext/openal-soft/build" }

    links { "rf", "openal", "glfw3", "GL", "X11", "dl", "pthread"  }
