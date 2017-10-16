#include "context.h"
#include "utils.h"
#include "sound.h"

bool static FramePressedKeys[350] = {};
bool static FrameReleasedKeys[350] = {};
bool static FrameDownKeys[350] = {};

int  static FrameModKeys = 0;

bool static FramePressedMouseButton[8] = {};
bool static FrameDownMouseButton[8] = {};
bool static FrameReleasedMouseButton[8] = {};

int  static FrameMouseWheel = 0;

int  static ResizeWidth;
int  static ResizeHeight;
bool static Resized = true;

void ProcessKeyboardEvent(GLFWwindow *Window, int Key, int Scancode, int Action, int Mods)
{
    if(Action == GLFW_PRESS)
    {
        FramePressedKeys[Key] = true;
        FrameDownKeys[Key] = true;
        FrameReleasedKeys[Key] = false;
    }
    if(Action == GLFW_RELEASE)
    {
        FramePressedKeys[Key] = false;
        FrameDownKeys[Key] = false;
        FrameReleasedKeys[Key] = true;
    }

    FrameModKeys = Mods;
}

void ProcessMouseButtonEvent(GLFWwindow* Window, int Button, int Action, int Mods)
{
    if(Action == GLFW_PRESS)
    {
        FramePressedMouseButton[Button] = true;
        FrameDownMouseButton[Button] = true;
        FrameReleasedMouseButton[Button] = false;
    }
    if(Action == GLFW_RELEASE)
    {
        FramePressedMouseButton[Button] = false;
        FrameDownMouseButton[Button] = false;
        FrameReleasedMouseButton[Button] = true;
    }

    FrameModKeys = Mods;
}

void ProcessMouseWheelEvent(GLFWwindow *Window, double XOffset, double YOffset)
{
    FrameMouseWheel = (int) YOffset;
}

void ProcessWindowSizeEvent(GLFWwindow *Window, int Width, int Height)
{
    Resized = true;
    ResizeWidth = Width;
    ResizeHeight = Height;
}

void ProcessErrorEvent(int Error, const char* Description)
{
    printf("GLFW Error : %s\n", Description);
}

key_state BuildKeyState(int32 Key)
{
    key_state State = 0;
    State |= (FramePressedKeys[Key] << 0x1);
    State |= (FrameReleasedKeys[Key] << 0x2);
    State |= (FrameDownKeys[Key] << 0x3);

    return State;
}

mouse_state BuildMouseState(int32 MouseButton)
{
    mouse_state State = 0;
    State |= (FramePressedMouseButton[MouseButton] << 0x1);
    State |= (FrameReleasedMouseButton[MouseButton] << 0x2);
    State |= (FrameDownMouseButton[MouseButton] << 0x3);

    return State;
}

namespace Context {
void RegisterShader3D(game_context *Context, uint32 ProgramID)
{
    Assert(Context->Shaders3DCount < MAX_SHADERS);
    Context->Shaders3D[Context->Shaders3DCount++] = ProgramID;
}

void RegisterShader2D(game_context *Context, uint32 ProgramID)
{
    Assert(Context->Shaders2DCount < MAX_SHADERS);
    Context->Shaders2D[Context->Shaders2DCount++] = ProgramID;
}

void RegisteredShaderClear(game_context *Context)
{
    Context->Shaders3DCount = 0;
    Context->Shaders2DCount = 0;
}

void UpdateShaderProjection(game_context *Context)
{
    // Notify the shaders that uses it
    for(uint32 i = 0; i < Context->Shaders3DCount; ++i)
    {
        glUseProgram(Context->Shaders3D[i]);
        SendMat4(glGetUniformLocation(Context->Shaders3D[i], "ProjMatrix"), Context->ProjectionMatrix3D);
    }

    for(uint32 i = 0; i < Context->Shaders2DCount; ++i)
    {
        glUseProgram(Context->Shaders2D[i]);
        SendMat4(glGetUniformLocation(Context->Shaders2D[i], "ProjMatrix"), Context->ProjectionMatrix2D);
    }
}

void WindowResized(game_context *Context)
{
    if(Resized)
    {
        Resized = false;

        glViewport(0, 0, ResizeWidth, ResizeHeight);
        Context->WindowWidth = ResizeWidth;
        Context->WindowHeight = ResizeHeight;
        Context->ProjectionMatrix3D = mat4f::Perspective(Context->FOV, 
                Context->WindowWidth / (real32)Context->WindowHeight, 0.1f, 10000.f);
        Context->ProjectionMatrix2D = mat4f::Ortho(0, Context->WindowWidth, 0, Context->WindowHeight, 0.1f, 1000.f);

        UpdateShaderProjection(Context);
    }
}

game_context Init(game_memory *Memory)
{
    game_context Context = {};
    game_config &Config = Memory->Config;

    bool GLFWValid = false, GLEWValid = false, SoundValid = false;

    GLFWValid = glfwInit();
    if(GLFWValid)
    {
        char WindowName[64];
        snprintf(WindowName, 64, "Radar v%d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH);

        Context.Window = glfwCreateWindow(Config.WindowWidth, Config.WindowHeight, WindowName, NULL, NULL);
        if(Context.Window)
        {
            glfwMakeContextCurrent(Context.Window);

            // TODO - Only in windowed mode for debug
		    glfwSetWindowPos(Context.Window, Config.WindowX, Config.WindowY);
            glfwSwapInterval(Config.VSync);

            glfwSetKeyCallback(Context.Window, ProcessKeyboardEvent);
            glfwSetMouseButtonCallback(Context.Window, ProcessMouseButtonEvent);
            glfwSetScrollCallback(Context.Window, ProcessMouseWheelEvent);
            glfwSetWindowSizeCallback(Context.Window, ProcessWindowSizeEvent);
            glfwSetErrorCallback(ProcessErrorEvent);

            GLEWValid = (GLEW_OK == glewInit());
            if(GLEWValid)
            {
                GLubyte const *GLRenderer = glGetString(GL_RENDERER);
                GLubyte const *GLVersion = glGetString(GL_VERSION);
                printf("GL Renderer %s, %s\n", GLRenderer, GLVersion);

                int MaxLayers;
                glGetIntegerv(GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS, &MaxLayers);
                printf("GL Max Array Layers : %d\n", MaxLayers);

                int MaxSize;
                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxSize);
                printf("GL Max Texture Width : %d\n", MaxSize);

                ResizeWidth = Config.WindowWidth;
                ResizeHeight = Config.WindowHeight;
                Context.WindowWidth = Config.WindowWidth;
                Context.WindowHeight = Config.WindowHeight;
                Context.FOV = Config.FOV;
                Context.ProjectionMatrix3D = mat4f::Perspective(Config.FOV, 
                        Config.WindowWidth / (real32)Config.WindowHeight, 0.1f, 10000.f);
                Context.ProjectionMatrix2D = mat4f::Ortho(0, Config.WindowWidth, 0,Config.WindowHeight, 0.1f, 1000.f);

                Context.WireframeMode = false;
                Context.ClearColor = vec4f(0.01f, 0.19f, 0.31f, 0.f);
                Context.GameConfig = &Config;

                glClearColor(Context.ClearColor.x, Context.ClearColor.y, Context.ClearColor.z, Context.ClearColor.w);

                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                glFrontFace(GL_CCW);

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glEnable(GL_POINT_SPRITE);
                glEnable(GL_PROGRAM_POINT_SIZE);

                {
                    path TexPath;
                    image Image;

                    MakeRelativePath(TexPath, Memory->ExecutableFullPath, "data/default_diffuse.png");
                    Image = ResourceLoadImage(Memory->ExecutableFullPath, TexPath, false);
                    Context.DefaultDiffuseTexture = Make2DTexture(&Image, false, false, 1);
                    DestroyImage(&Image);

                    MakeRelativePath(TexPath, Memory->ExecutableFullPath, "data/default_normal.png");
                    Image = ResourceLoadImage(Memory->ExecutableFullPath, TexPath, false);
                    Context.DefaultNormalTexture = Make2DTexture(&Image, false, false, 1);
                    DestroyImage(&Image);
#if RADAR_WIN32
                    Context.DefaultFont = ResourceLoadFont(Memory, "C:/Windows/Fonts/dejavusansmono.ttf", 24);
#else
                    Context.DefaultFont = ResourceLoadFont(Memory, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 24);
#endif
                    Context.DefaultFontTexture = Context.DefaultFont.AtlasTextureID;
                }
            }
            else
            {
                printf("Couldn't initialize GLEW.\n");
            }
        }
        else
        {
            printf("Couldn't create GLFW Window.\n");
        }
    }
    else
    {
        printf("Couldn't init GLFW.\n");
    }

    SoundValid = sound::Init();

    if(GLFWValid && GLEWValid && SoundValid)
    {
        // NOTE - IsRunning might be better elsewhere ?
        Context.IsRunning = true;
        Context.IsValid = true;
    }

    return Context;
}

void GetFrameInput(game_context *Context, game_input *Input)
{
    memset(FrameReleasedKeys, 0, sizeof(FrameReleasedKeys));
    memset(FramePressedKeys, 0, sizeof(FramePressedKeys));
    memset(FrameReleasedMouseButton, 0, sizeof(FrameReleasedMouseButton));
    memset(FramePressedMouseButton, 0, sizeof(FramePressedMouseButton));

    FrameMouseWheel = 0;

    glfwPollEvents();

    // NOTE - The mouse position can go outside the Window bounds in windowed mode
    // See if this can cause problems in the future.
    real64 MX, MY;
    glfwGetCursorPos(Context->Window, &MX, &MY);
    Input->MousePosX = (int)MX;
    Input->MousePosY = (int)MY;

    if(glfwWindowShouldClose(Context->Window))
    {
        Context->IsRunning = false;
    }

    if(FrameReleasedKeys[GLFW_KEY_ESCAPE])
    {
        Context->IsRunning = false;
    }

    // Get Player controls
    Input->KeyW = BuildKeyState(GLFW_KEY_W);
    Input->KeyA = BuildKeyState(GLFW_KEY_A);
    Input->KeyS = BuildKeyState(GLFW_KEY_S);
    Input->KeyD = BuildKeyState(GLFW_KEY_D);
    Input->KeyR = BuildKeyState(GLFW_KEY_R);
    Input->KeyF = BuildKeyState(GLFW_KEY_F);
    Input->KeyLShift = BuildKeyState(GLFW_KEY_LEFT_SHIFT);
    Input->KeyLCtrl = BuildKeyState(GLFW_KEY_LEFT_CONTROL);
    Input->KeyLAlt = BuildKeyState(GLFW_KEY_LEFT_ALT);
    Input->KeySpace = BuildKeyState(GLFW_KEY_SPACE);
    Input->KeyF1 = BuildKeyState(GLFW_KEY_F1);
    Input->KeyF2 = BuildKeyState(GLFW_KEY_F2);
    Input->KeyF11 = BuildKeyState(GLFW_KEY_F11);
    Input->KeyNumPlus = BuildKeyState(GLFW_KEY_KP_ADD);
    Input->KeyNumMinus = BuildKeyState(GLFW_KEY_KP_SUBTRACT);
    Input->KeyNumMultiply = BuildKeyState(GLFW_KEY_KP_MULTIPLY);
    Input->KeyNumDivide = BuildKeyState(GLFW_KEY_KP_DIVIDE);

    Input->MouseLeft = BuildMouseState(GLFW_MOUSE_BUTTON_LEFT);
    Input->MouseRight = BuildMouseState(GLFW_MOUSE_BUTTON_RIGHT);

    Input->dTimeFixed = 0.1f; // 100FPS
}

void Destroy(game_context *Context)
{
    sound::Destroy();

    if(Context->Window)
    {
        glfwDestroyWindow(Context->Window);
    }
    glfwTerminate();
}
}