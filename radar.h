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


#ifdef RADAR_WIN32
// library loading
#define dll_export __declspec(dllexport)
#endif

#ifdef RADAR_UNIX
#define dll_export 
#endif


dll_export
int TestAdd(int a, int b);

#endif
