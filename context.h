#ifndef CONTEXT_H
#define CONTEXT_H

#include "render.h"

#if RADAR_WIN32
#define NOMINMAX
#include <windows.h>
#else
#endif
#include "GLFW/glfw3.h"

#define MAX_SHADERS 32

struct game_context
{
    GLFWwindow *Window;

    mat4f ProjectionMatrix3D;
    mat4f ProjectionMatrix2D;

    bool WireframeMode;
    vec4f ClearColor;

    uint32 DefaultDiffuseTexture;
    uint32 DefaultNormalTexture;

    font   DefaultFont;
    uint32 DefaultFontTexture;

    real32 FOV;
    int WindowWidth;
    int WindowHeight;

    uint32 Shaders3D[MAX_SHADERS];
    uint32 Shaders3DCount;
    uint32 Shaders2D[MAX_SHADERS];
    uint32 Shaders2DCount;

    game_config *GameConfig;

    bool IsRunning;
    bool IsValid;
};

void RegisterShader3D(game_context *Context, uint32 ProgramID);
void RegisterShader2D(game_context *Context, uint32 ProgramID);
#endif
