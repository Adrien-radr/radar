#ifndef RADAR_COMMON
#define RADAR_COMMON

//////////////////////////////////////////////////////////////////////////
// NOTE - This Header contains whatever both the Game DLL AND the Platform
// should have access to. In general : very common things (typedefs, 
// defines, macros, common headers, math, etc).
//////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <cstring>
#include <vector>
#include "linmath.h"

#define RADAR_MAJOR 0
#define RADAR_MINOR 0
#define RADAR_PATCH 1

// Platform
#if defined(_WIN32) || defined(_WIN64)
#   define RADAR_WIN32 1
#   define DLLEXPORT extern "C" __declspec(dllexport)
#elif defined(__unix__) || defined (__unix) || defined(unix)
#   define RADAR_UNIX 1
#   define DLLEXPORT extern "C"
#   include <stddef.h>
#else
#   error "Unknown OS. Only Windows & Linux supported for now."
#endif

typedef float real32;
typedef double real64;

typedef char                int8;
typedef unsigned char       uint8;
typedef short int           int16;
typedef unsigned short int  uint16;
typedef int                 int32;
typedef unsigned int        uint32;
typedef long long           int64;
typedef unsigned long long  uint64;

#define MAX_PATH 260
typedef char path[MAX_PATH];

#ifdef DEBUG
#include <stdio.h>
#ifndef Assert
#define Assert(expr) if(!(expr)) { printf("Assert %s %d.\n", __FILE__, __LINE__); *(int*)0 = 0; }
#endif
#define DebugPrint(str, ...) printf(str, ##__VA_ARGS__);
#else
#ifndef Assert
#define Assert(expr) 
#endif
#define DebugPrint(str, ...)
#endif

#define Kilobytes(num) (1024LL*(num))
#define Megabytes(num) (1024LL*Kilobytes(num))
#define Gigabytes(num) (1024LL*Megabytes(num))


#endif
