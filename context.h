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
    render_resources RenderResources;

    mat4f ProjectionMatrix3D;
    mat4f ProjectionMatrix2D;

    bool WireframeMode;
    vec4f ClearColor;

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

namespace context
{
    enum cursor_type
    {
        CURSOR_NORMAL,
        CURSOR_HRESIZE,
        CURSOR_VRESIZE
    };

    game_context *Init(game_memory *Memory);
    void Destroy(game_context *Context);

    bool WindowResized(game_context *Context);
    void UpdateShaderProjection(game_context *Context);
    void GetFrameInput(game_context *Context, game_input *Input);

    void RegisteredShaderClear(game_context *Context);
    void RegisterShader3D(game_context *Context, uint32 ProgramID);
    void RegisterShader2D(game_context *Context, uint32 ProgramID);

    void SetCursor(game_context *Context, cursor_type CursorType);
}


#endif
