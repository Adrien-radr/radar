#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "utils.h"
#include "context.h"
#include "ui.h"
#include "water.h"
#include "sun.h"

// PLATFORM
int RadarMain(int argc, char **argv);
#if RADAR_WIN32
#include "radar_win32.cpp"
#elif RADAR_UNIX
#include "radar_unix.cpp"
#endif

game_memory InitMemory()
{
    game_memory Memory = {};

    Memory.PermanentMemPoolSize = Megabytes(32);
    Memory.SessionMemPoolSize = Megabytes(512);
    Memory.ScratchMemPoolSize = Megabytes(64);

    Memory.PermanentMemPool = calloc(1, Memory.PermanentMemPoolSize);
    Memory.SessionMemPool = calloc(1, Memory.SessionMemPoolSize);
    Memory.ScratchMemPool = calloc(1, Memory.ScratchMemPoolSize);

    InitArena(&Memory.SessionArena, Memory.SessionMemPoolSize, Memory.SessionMemPool);
    InitArena(&Memory.ScratchArena, Memory.ScratchMemPoolSize, Memory.ScratchMemPool);

    Memory.IsValid = Memory.PermanentMemPool && Memory.SessionMemPool && Memory.ScratchMemPool;
    Memory.IsInitialized = false;

    return Memory;
}

void DestroyMemory(game_memory *Memory)
{
    if(Memory->IsValid)
    {
        free(Memory->PermanentMemPool);
        free(Memory->SessionMemPool);
        free(Memory->ScratchMemPool);
        Memory->PermanentMemPoolSize = 0;
        Memory->SessionMemPoolSize = 0;
        Memory->ScratchMemPoolSize = 0;
        Memory->IsValid = false;
        Memory->IsInitialized = false;
        Memory->IsGameInitialized = false;
    }
}

void ParseConfig(game_memory *Memory, char *ConfigPath)
{
    game_config &Config = Memory->Config;

    void *Content = ReadFileContents(&Memory->ScratchArena, ConfigPath, 0);
    if(Content)
    {
        cJSON *root = cJSON_Parse((char*)Content);
        if(root)
        {
            Config.WindowX = JSON_Get(root, "iWindowX", 200);
            Config.WindowY = JSON_Get(root, "iWindowY", 200);
            Config.WindowWidth = JSON_Get(root, "iWindowWidth", 960);
            Config.WindowHeight = JSON_Get(root, "iWindowHeight", 540);
            Config.MSAA = JSON_Get(root, "iMSAA", 0);
            Config.FullScreen = JSON_Get(root, "bFullScreen", 0) != 0;
            Config.VSync = JSON_Get(root, "bVSync", 0) != 0;
            Config.FOV = (real32)JSON_Get(root, "fFOV", 75.0);
            Config.AnisotropicFiltering = JSON_Get(root, "iAnisotropicFiltering", 1);

            Config.CameraSpeedBase = (real32)JSON_Get(root, "fCameraSpeedBase", 20.0);
            Config.CameraSpeedMult = (real32)JSON_Get(root, "fCameraSpeedMult", 2.0);
            Config.CameraSpeedAngular = (real32)JSON_Get(root, "fCameraSpeedAngular", 0.3);

            Config.CameraPosition = JSON_Get(root, "vCameraPosition", vec3f(1,1,1));
            Config.CameraTarget = JSON_Get(root, "vCameraTarget", vec3f(0,0,0));

            Config.DebugFontColor = col4f(JSON_Get(root, "vDebugFontColor", vec3f(1,0,0)));
        }
        else
        {
            printf("Error parsing Config File as JSON.\n");
        }
    }
    else
    {
        Config.WindowWidth = 960;
        Config.WindowHeight = 540;
        Config.MSAA = 0;
        Config.FullScreen = false;
        Config.VSync = false;
        Config.FOV = 75.f;
        Config.AnisotropicFiltering = 1;

        Config.CameraSpeedBase = 20.f;
        Config.CameraSpeedMult = 2.f;
        Config.CameraSpeedAngular = 0.3f;
        Config.CameraPosition = vec3f(1, 1, 1);
        Config.CameraTarget = vec3f(0, 0, 0);
    }
}

uint32 Program1, Program3D, ProgramSkybox;

void ReloadShaders(game_memory *Memory, game_context *Context)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;
	path &ExecutableFullPath = Memory->ExecutableFullPath;

    Context::RegisteredShaderClear(Context);

    path VSPath;
    path FSPath;
    MakeRelativePath(VSPath, ExecutableFullPath, "data/shaders/text_vert.glsl");
    MakeRelativePath(FSPath, ExecutableFullPath, "data/shaders/text_frag.glsl");
    Program1 = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program1);
    SendInt(glGetUniformLocation(Program1, "DiffuseTexture"), 0);
    CheckGLError("Text Shader");

    Context::RegisterShader2D(Context, Program1);

    MakeRelativePath(VSPath, ExecutableFullPath, "data/shaders/vert.glsl");
    MakeRelativePath(FSPath, ExecutableFullPath, "data/shaders/frag.glsl");
    Program3D = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program3D);
    SendInt(glGetUniformLocation(Program3D, "Albedo"), 0);
    SendInt(glGetUniformLocation(Program3D, "MetallicRoughness"), 1);
    SendInt(glGetUniformLocation(Program3D, "Skybox"), 2);
    SendInt(glGetUniformLocation(Program3D, "IrradianceCubemap"), 3);
    CheckGLError("Mesh Shader");

    Context::RegisterShader3D(Context, Program3D);

    MakeRelativePath(VSPath, ExecutableFullPath, "data/shaders/skybox_vert.glsl");
    MakeRelativePath(FSPath, ExecutableFullPath, "data/shaders/skybox_frag.glsl");
    ProgramSkybox = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramSkybox);
    SendInt(glGetUniformLocation(ProgramSkybox, "Skybox"), 0);
    CheckGLError("Skybox Shader");

    Context::RegisterShader3D(Context, ProgramSkybox);

    MakeRelativePath(VSPath, ExecutableFullPath, "data/shaders/water_vert.glsl");
    MakeRelativePath(FSPath, ExecutableFullPath, "data/shaders/water_frag.glsl");
    System->WaterSystem->ProgramWater = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(System->WaterSystem->ProgramWater);
    SendInt(glGetUniformLocation(System->WaterSystem->ProgramWater, "Skybox"), 0);
    SendInt(glGetUniformLocation(System->WaterSystem->ProgramWater, "IrradianceCubemap"), 1);
    CheckGLError("Water Shader");

    Context::RegisterShader3D(Context, System->WaterSystem->ProgramWater);

    uiReloadShaders(Memory, Context, ExecutableFullPath);
    CheckGLError("UI Shader");

    Context::UpdateShaderProjection(Context);
    
    glUseProgram(0);
}

void InitializeFromGame(game_memory *Memory)
{
    // Initialize Water from game Info
    game_system *System = (game_system*)Memory->PermanentMemPool;
    game_state *State = (game_state*)POOL_OFFSET(Memory->PermanentMemPool, game_system);

    WaterInitialization(Memory, State, System, State->WaterState);

    water_system *WaterSystem = System->WaterSystem;
    WaterSystem->VAO = MakeVertexArrayObject();
    WaterSystem->VBO[0] = AddIBO(GL_STATIC_DRAW, WaterSystem->IndexCount * sizeof(uint32), WaterSystem->IndexData);
    WaterSystem->VBO[1] = AddEmptyVBO(WaterSystem->VertexDataSize, GL_STATIC_DRAW);
    size_t VertSize = WaterSystem->VertexCount * sizeof(real32);
    FillVBO(0, 3, GL_FLOAT, 0, VertSize, WaterSystem->VertexData);
    FillVBO(1, 3, GL_FLOAT, VertSize, VertSize, WaterSystem->VertexData + WaterSystem->VertexCount);
    FillVBO(2, 3, GL_FLOAT, 2*VertSize, VertSize, WaterSystem->VertexData + 2 * WaterSystem->VertexCount);
    glBindVertexArray(0);

    Memory->IsGameInitialized = true;
    Memory->IsInitialized = true;
}

void MakeUI(game_memory *Memory, game_context *Context, font *Font)
{
    game_system *System = (game_system*)Memory->PermanentMemPool;

    console_log *Log = System->ConsoleLog;
    for(uint32 i = 0; i < Log->StringCount; ++i)
    {
        uint32 RIdx = (Log->ReadIdx + i) % CONSOLE_CAPACITY;
        uiMakeText(Log->MsgStack[RIdx], Font, vec3f(10, 10 + i * Font->LineGap, 0), 
                Context->GameConfig->DebugFontColor, Context->WindowWidth - 10);
    }

    ui_frame_stack *UIStack = System->UIStack;
    for(uint32 i = 0; i < UIStack->TextLineCount; ++i)
    {
        ui_text_line *Line = &UIStack->TextLines[i];
        // TODO - handle different fonts
        uiMakeText(Line->String, Font, Line->Position, Line->Color, Context->WindowWidth);
    }

#if 0
    uiBeginPanel("Title is a pretty long sentence", vec3i(500, 100, 0), vec2i(200, 100), col4f(0,1,0,0.5));
    uiEndPanel();

    uiBeginPanel("Title2", vec3i(600, 150, 1), vec2i(200, 100), col4f(1,0,0,0.5));
    uiEndPanel();
#endif
}

int RadarMain(int argc, char **argv)
{
    path DllSrcPath;
    path DllDstPath;

    game_memory Memory = InitMemory();

    GetExecutablePath(Memory.ExecutableFullPath);
    MakeRelativePath(DllSrcPath, Memory.ExecutableFullPath, DllName);
    MakeRelativePath(DllDstPath, Memory.ExecutableFullPath, DllDynamicCopyName);

    path ConfigPath;
    MakeRelativePath(ConfigPath, Memory.ExecutableFullPath, "config.json");

    ParseConfig(&Memory, ConfigPath);
    game_context Context = Context::Init(&Memory);
    game_code Game = LoadGameCode(DllSrcPath, DllDstPath);
    game_config const &Config = Memory.Config;

    if(Context.IsValid && Memory.IsValid)
    {
        real64 CurrentTime, LastTime = glfwGetTime();
        int GameRefreshHz = 60;
        real64 TargetSecondsPerFrame = 1.0 / (real64)GameRefreshHz;

        uiInit(&Context);

        game_system *System = (game_system*)Memory.PermanentMemPool;
        game_state *State = (game_state*)POOL_OFFSET(Memory.PermanentMemPool, game_system);

        System->ConsoleLog = (console_log*)PushArenaStruct(&Memory.SessionArena, console_log);
        System->SoundData = (tmp_sound_data*)PushArenaStruct(&Memory.SessionArena, tmp_sound_data);

        //ReloadShaders(&Memory, &Context, Memory.ExecutableFullPath);
        glActiveTexture(GL_TEXTURE0);
        CheckGLError("Start");
/////////////////////////
    // TEMP TESTS
#if 0
        ALuint AudioBuffer;
        ALuint AudioSource;
        if(TempPrepareSound(&AudioBuffer, &AudioSource))
        {
            alSourcePlay(AudioSource);
        }
#endif

        // Texture Test
        path TexPath;
        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/crate1_diffuse.png");
        image Image = ResourceLoadImage(Memory.ExecutableFullPath, TexPath, false);
        uint32 Texture1 = Make2DTexture(&Image, false, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

#if 0
        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/albedo.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalAlbedo = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/metallic.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalMetallic = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/roughness.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalRoughness = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);

        MakeRelativePath(TexPath, Memory.ExecutableFullPath, "data/brick_1/normal.png");
        Image = ResourceLoadImage(TexPath);
        uint32 RustedMetalNormal = Make2DTexture(&Image, false, Config.AnisotropicFiltering);
        DestroyImage(&Image);
#endif
        // Font Test
#if RADAR_WIN32
        font ConsoleFont = ResourceLoadFont(&Memory, "C:/Windows/Fonts/dejavusansmono.ttf", 14);
#else
        font ConsoleFont = ResourceLoadFont(&Memory, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
#endif

        // Cube Meshes Test
        mesh Cube = MakeUnitCube();
        real32 Dim = 20.0f;
        vec3f LowDim = -Dim/2;
        vec3f CubePos[] = {
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX),
            LowDim + vec3f(Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX, Dim*rand()/(real32)RAND_MAX)
        };
        vec3f CubeRot[] = {
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX),
            vec3f(2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX, 2.f*M_PI*rand()/(real32)RAND_MAX)
        };
        mesh Sphere = MakeUnitSphere();


        // Cubemaps Test
        path CubemapPaths[6];
        {
#if 0
            path CubemapNames[6] = {
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
                "data/default_diffuse.png",
            };
#else
            path CubemapNames[6] = {
                "data/Skybox/1/right.png",
                "data/Skybox/1/left.png",
                "data/Skybox/1/bottom.png",
                "data/Skybox/1/top.png",
                "data/Skybox/1/back.png",
                "data/Skybox/1/front.png",
            };
#endif
            for(uint32 i = 0; i < 6; ++i)
            {
                MakeRelativePath(CubemapPaths[i], Memory.ExecutableFullPath, CubemapNames[i]);
            }
        }

        mesh SkyboxCube = MakeUnitCube(false);
        uint32 TestCubemap = MakeCubemap(Memory.ExecutableFullPath, CubemapPaths, false, false, 0, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, TestCubemap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, TestCubemap);
        glActiveTexture(GL_TEXTURE0);

        uint32 HDRCubemapEnvmap, HDRIrradianceEnvmap;
        ComputeIrradianceCubemap(&Memory, Memory.ExecutableFullPath, "data/envmap_monument.hdr", &HDRCubemapEnvmap, &HDRIrradianceEnvmap);
        uint32 EnvmapToUse = HDRIrradianceEnvmap; 

        model gltfCube = {};
        mesh defCube = MakeUnitCube();
        //if(!ResourceLoadGLTFModel(&gltfCube, "data/gltftest/PBRSpheres/MetalRoughSpheres.gltf", &Context))
        if(!ResourceLoadGLTFModel(&gltfCube, "data/gltftest/suzanne/Suzanne.gltf", &Context))
            return 1;
         

        bool LastDisableMouse = false;

        while(Context.IsRunning)
        {
            game_input Input = {};

            CurrentTime = glfwGetTime();
            Input.dTime = CurrentTime - LastTime;

            LastTime = CurrentTime;
            State->EngineTime += Input.dTime;

            // NOTE - Each frame, clear the Scratch Arena Data
            // TODO - Is this too often ? Maybe let it stay several frames
            ClearArena(&Memory.ScratchArena);

            Context::GetFrameInput(&Context, &Input);        
            Context::WindowResized(&Context);

            if(CheckNewDllVersion(&Game, DllSrcPath))
            {
                UnloadGameCode(&Game, NULL);
                Game = LoadGameCode(DllSrcPath, DllDstPath);
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            uiBeginFrame(&Memory, &Input);
            Game.GameUpdate(&Memory, &Input);
            if(!Memory.IsInitialized)
            {
                InitializeFromGame(&Memory);
				ReloadShaders(&Memory, &Context);			// First Shader loading after the game is initialized
            }

#if 0
                tmp_sound_data *SoundData = System->SoundData;
                if(SoundData->ReloadSoundBuffer)
                {
                    SoundData->ReloadSoundBuffer = false;
                    alSourceStop(AudioSource);
                    alSourcei(AudioSource, AL_BUFFER, 0);
                    alBufferData(AudioBuffer, AL_FORMAT_MONO16, SoundData->SoundBuffer, SoundData->SoundBufferSize, 48000);
                    alSourcei(AudioSource, AL_BUFFER, AudioBuffer);
                    alSourcePlay(AudioSource);
                    CheckALError();
                }
#endif

            if(LastDisableMouse != State->DisableMouse)
            {
                if(State->DisableMouse)
                {
                    glfwSetInputMode(Context.Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
                else
                {
                    glfwSetInputMode(Context.Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }

                LastDisableMouse = State->DisableMouse;
            }

            if(KEY_DOWN(Input.KeyLShift) && KEY_UP(Input.KeyF11))
            {
                ReloadShaders(&Memory, &Context);
            }

            if(KEY_UP(Input.KeyF1))
            {
                Context.WireframeMode = !Context.WireframeMode;
                glPolygonMode(GL_FRONT_AND_BACK, Context.WireframeMode ? GL_LINE : GL_FILL);
            }

            if(KEY_UP(Input.KeyF2))
            {
                if(EnvmapToUse == HDRCubemapEnvmap)
                    EnvmapToUse = HDRIrradianceEnvmap;
                else
                    EnvmapToUse = HDRCubemapEnvmap;
            }


            // Recompute Camera View matrix if needed
            game_camera &Camera = State->Camera;
            mat4f &ViewMatrix = Camera.ViewMatrix;
            ViewMatrix = mat4f::LookAt(Camera.Position, Camera.Target, Camera.Up);


            {
                glCullFace(GL_BACK);
                glUseProgram(Program3D);
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }

                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);
                uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
                uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
                uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
                for(uint32 i = 0; i < gltfCube.Mesh.size(); ++i)
                {
                    material const &Mat = gltfCube.Material[gltfCube.MaterialIdx[i]];

                    SendVec3(AlbedoLoc, Mat.AlbedoMult);
                    SendFloat(MetallicLoc, Mat.MetallicMult);
                    SendFloat(RoughnessLoc, Mat.RoughnessMult);
                    glBindVertexArray(gltfCube.Mesh[i].VAO);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Mat.AlbedoTexture);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Mat.RoughnessMetallicTexture);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, EnvmapToUse);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, HDRIrradianceEnvmap);
                    mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
                    Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                    ModelMatrix.FromTRS(vec3f(0), vec3f(0), vec3f(2));
                    SendMat4(Loc, ModelMatrix);
                    glDrawElements(GL_TRIANGLES, gltfCube.Mesh[i].IndexCount, gltfCube.Mesh[i].IndexType, 0);
                }
            }

#if 0
            { // NOTE - CUBE DRAWING Test Put somewhere else
                glUseProgram(Program3D);
                {

                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }

                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);
                glBindVertexArray(Cube.VAO);
                glBindTexture(GL_TEXTURE_2D, Texture1);
                mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                for(uint32 i = 0; i < 5; ++i)
                {
                    ModelMatrix.FromTRS(CubePos[i], CubeRot[i], vec3f(1.f));
                    SendMat4(Loc, ModelMatrix);
                    CheckGLError("ModelMatrix");
                    glDrawElements(GL_TRIANGLES, Cube.IndexCount, GL_UNSIGNED_INT, 0);
                }

                real32 hW = PlaneWidth/2.f;
                ModelMatrix.FromTRS(vec3f(-hW, -7.f, -hW), vec3f(0), vec3f(1));
                SendMat4(Loc, ModelMatrix);
                glBindVertexArray(UnderPlane.VAO);
                glDrawElements(GL_TRIANGLES, UnderPlane.IndexCount, GL_UNSIGNED_INT, 0);
            }
#endif

#if 1
            { // NOTE - Sphere Array Test for PBR
                glUseProgram(Program3D);
                {
                    uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
                    SendMat4(Loc, ViewMatrix);
                    CheckGLError("ViewMatrix");
                }
                uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
                SendVec4(Loc, State->LightColor);
                Loc = glGetUniformLocation(Program3D, "SunDirection");
                SendVec3(Loc, State->LightDirection);
                Loc = glGetUniformLocation(Program3D, "CameraPos");
                SendVec3(Loc, State->Camera.Position);

                glBindVertexArray(Sphere.VAO);
                mat4f ModelMatrix;
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Context.DefaultDiffuseTexture);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Context.DefaultDiffuseTexture);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_CUBE_MAP, EnvmapToUse);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_CUBE_MAP, HDRIrradianceEnvmap);

                int Count = 5;
                uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
                uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
                uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
                SendVec3(AlbedoLoc, vec3f(1));
                for(int j = 0; j < Count; ++j)
                {
                    SendFloat(MetallicLoc, (j+1)/(real32)Count);
                    for(int i = 0; i < Count; ++i)
                    {
                        SendFloat(RoughnessLoc, (i+1)/(real32)Count);
                        ModelMatrix.FromTRS(vec3f(0, 3*(j+1), 3*(i+1)), vec3f(0.f), vec3f(1.f));
                        SendMat4(Loc, ModelMatrix);
                        glDrawElements(GL_TRIANGLES, Sphere.IndexCount, GL_UNSIGNED_INT, 0);
                    }
                }
                glActiveTexture(GL_TEXTURE0);
            }
#endif

#if 1
            WaterUpdate(State, System->WaterSystem, &Input);
            WaterRender(State, System->WaterSystem, EnvmapToUse, HDRIrradianceEnvmap);
#endif

            { // NOTE - Skybox Rendering Test, put somewhere else
                glDisable(GL_CULL_FACE);
                glDepthFunc(GL_LEQUAL);
                CheckGLError("Skybox");

                glUseProgram(ProgramSkybox);
                {
                    // NOTE - remove translation component from the ViewMatrix for the skybox
                    mat4f SkyViewMatrix = ViewMatrix;
                    SkyViewMatrix.SetTranslation(vec3f(0.f));
                    uint32 Loc = glGetUniformLocation(ProgramSkybox, "ViewMatrix");
                    SendMat4(Loc, SkyViewMatrix);
                    CheckGLError("ViewMatrix Skybox");
                }
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, EnvmapToUse);
                glBindVertexArray(SkyboxCube.VAO);
                glDrawElements(GL_TRIANGLES, SkyboxCube.IndexCount, GL_UNSIGNED_INT, 0);

                glDepthFunc(GL_LESS);
                glEnable(GL_CULL_FACE);
            }

            MakeUI(&Memory, &Context, &ConsoleFont);
            uiDraw();

            glfwSwapBuffers(Context.Window);
        }

        // TODO - Destroy Console meshes
        DestroyMesh(&Cube);
        DestroyMesh(&SkyboxCube);
        glDeleteTextures(1, &Texture1);
        glDeleteProgram(Program1);
        glDeleteProgram(Program3D);
        glDeleteProgram(ProgramSkybox);
    }

    DestroyMemory(&Memory);
    Context::Destroy(&Context);
    UnloadGameCode(&Game, DllDstPath);

    return 0;
}
