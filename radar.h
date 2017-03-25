#ifndef RADAR_H
#define RADAR_H

#define RADAR_MAJOR 0
#define RADAR_MINOR 0
#define RADAR_PATCH 1

// Platform
#if defined(_WIN32) || defined(_WIN64)
#   define RADAR_WIN32
#elif defined(__unix__) || defined (__unix) || defined(unix)
#   define RADAR_UNIX
#else
#   error "Unknown OS. Only Windows & Linux supported for now."
#endif

typedef float real32;
typedef double real64;
typedef unsigned int uint32;
typedef int int32;

// NOTE - struct passed to the Game
// Contains all frame input each frame needed by game
struct game_input
{
    real64 dTime;

    bool KeyReleased;
    bool KeyPressed;
    bool KeyDown;
};
#endif
