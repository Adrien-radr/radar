#include "tests.h"

#include "rf/utils.h"
#include "rf/context.h"

namespace Tests
{
// PBR Materials tests
uint32 Program3D = 0;
uint32 GGXLUT = 0;
uint32 HDRCubemapEnvmap, HDRIrradianceEnvmap, HDRGlossyEnvmap;
rf::mesh Sphere = {};
rf::mesh Cube = {};

// PBR GLTF Models tests
uint32 const PBRModelsCount = 1;
uint32 const PBRModelsCapacity = 2;
rf::model PBRModels[PBRModelsCapacity];
float rotation[PBRModelsCapacity] = { 0.f };
vec3f translation[PBRModelsCapacity] = { vec3f(-5.f, 0.f, 0.f) , vec3f(0,-1.98f,0) };
int translationDir[PBRModelsCapacity] = { 1 };

// PBR MATERIALS
uint32 *Texture1;
uint32 const MaterialCount = 6;
uint32 *PBR_Albedo[MaterialCount];
uint32 *PBR_MetalRoughness[MaterialCount];
uint32 *PBR_Normal[MaterialCount];

// SOUND
//#include "AL/al.h"
//ALuint AudioBuffer;
//ALuint AudioSource;

// SKYBOX 
rf::mesh SkyboxCube = {};
uint32 ProgramSkybox = 0;

void Destroy()
{
    glDeleteTextures(1, &GGXLUT);
    glDeleteTextures(1, &HDRCubemapEnvmap);
    glDeleteTextures(1, &HDRGlossyEnvmap);
    glDeleteTextures(1, &HDRIrradianceEnvmap);
    rf::DestroyMesh(&Sphere);
    rf::DestroyMesh(&Cube);
    rf::DestroyMesh(&SkyboxCube);
    glDeleteProgram(ProgramSkybox);
    glDeleteProgram(Program3D);
}

void ReloadShaders(rf::context *Context)
{
    path const &ExePath = rf::ctx::GetExePath(Context);
    path VSPath, FSPath;

    rf::ConcatStrings(VSPath, ExePath, "data/shaders/vert.glsl");
    rf::ConcatStrings(FSPath, ExePath, "data/shaders/frag.glsl");
    Program3D = rf::BuildShader(Context, VSPath, FSPath);
    glUseProgram(Program3D);
    rf::SendInt(glGetUniformLocation(Program3D, "Albedo"), 0);
    rf::SendInt(glGetUniformLocation(Program3D, "NormalMap"), 1);
    rf::SendInt(glGetUniformLocation(Program3D, "MetallicRoughness"), 2);
    rf::SendInt(glGetUniformLocation(Program3D, "Emissive"), 3);
    rf::SendInt(glGetUniformLocation(Program3D, "GGXLUT"), 4);
    rf::SendInt(glGetUniformLocation(Program3D, "Skybox"), 5);
    rf::CheckGLError("Mesh Shader");

    rf::ctx::RegisterShader3D(Context, Program3D);

    rf::ConcatStrings(VSPath, ExePath, "data/shaders/skybox_vert.glsl");
    rf::ConcatStrings(FSPath, ExePath, "data/shaders/skybox_frag.glsl");
    ProgramSkybox = rf::BuildShader(Context, VSPath, FSPath);
    glUseProgram(ProgramSkybox);
    rf::SendInt(glGetUniformLocation(ProgramSkybox, "Skybox"), 0);
    rf::CheckGLError("Skybox Shader");

    rf::ctx::RegisterShader3D(Context, ProgramSkybox);
}

bool Init(rf::context *Context, config const *Config)
{
    GGXLUT = rf::PrecomputeGGXLUT(Context, 512);
    rf::ComputeIrradianceCubemap(Context, "data/envmap_monument.hdr", 
                                &HDRCubemapEnvmap, &HDRGlossyEnvmap, &HDRIrradianceEnvmap);
    Sphere = rf::MakeUnitSphere(true, 3);
    Cube = rf::MakeUnitCube();
    SkyboxCube = rf::MakeUnitCube(false);

#if TEST_MODELS
    if(!rf::ResourceLoadGLTFModel(Context, &PBRModels[0], "data/gltftest/suzanne/Suzanne.gltf", Config->AnisotropicFiltering))
        return false;
    //if(!rf::ResourceLoadGLTFModel(Context, &PBRModels[1], "data/gltftest/lantern/Lantern.gltf", Config->AnisotropicFiltering))
    //return 1;
#endif

#if TEST_PBRMATERIALS
    // PBR Texture Tes
    Texture1 = rf::ResourceLoad2DTexture(Context, "data/crate1_diffuse.png",
            false, false, Config->AnisotropicFiltering);

    PBR_Albedo[0] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/StreakedMetal/albedo.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_MetalRoughness[0] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/StreakedMetal/metalroughness.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_Normal[0] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/StreakedMetal/normal.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);

    PBR_Albedo[1] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/ScuffedGold/albedo.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_MetalRoughness[1] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/ScuffedGold/metalroughness.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_Normal[1] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/ScuffedGold/normal.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);

    PBR_Albedo[2] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/RustedIron/albedo.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_MetalRoughness[2] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/RustedIron/metalroughness.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_Normal[2] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/RustedIron/normal.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);

    PBR_Albedo[3] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/ScuffedPlastic/albedo_blue.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_MetalRoughness[3] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/ScuffedPlastic/metalroughness.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_Normal[3] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/ScuffedPlastic/normal.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);

    PBR_Albedo[4] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/SynthRubber/albedo.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_MetalRoughness[4] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/SynthRubber/metalroughness.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_Normal[4] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/SynthRubber/normal.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);

    PBR_Albedo[5] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/PlasticPattern/albedo.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_MetalRoughness[5] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/PlasticPattern/metalroughness.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
    PBR_Normal[5] = rf::ResourceLoad2DTexture(Context, "data/PBRTextures/PlasticPattern/normal.png",
            false, false, Config->AnisotropicFiltering, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT, GL_REPEAT);
#endif

#if TEST_SOUND
    if(TempPrepareSound(&AudioBuffer, &AudioSource))
    {
        alSourcePlay(AudioSource);
    }
#endif
    return true;
}

void Render(game::state *State, rf::input *Input, rf::context *Context)
{
    mat4f const &ViewMatrix = State->Camera.ViewMatrix;

#if TEST_MODELS // NOTE - Model rendering test
    {
        glCullFace(GL_BACK);
        glUseProgram(Program3D);
        {
            uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
            rf::SendMat4(Loc, ViewMatrix);
            rf::CheckGLError("ViewMatrix");
        }

        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        rf::SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        rf::SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        rf::SendVec3(Loc, State->Camera.Position);
        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
        for(uint32  m = 0; m < PBRModelsCount; ++m)
        {
            rf::model *Model = &PBRModels[m];
            for(uint32 i = 0; i < Model->Mesh.size(); ++i)
            {
                rf::material const &Mat = Model->Material[Model->MaterialIdx[i]];

                rf::SendVec3(AlbedoLoc, Mat.AlbedoMult);
                rf::SendVec3(EmissiveLoc, Mat.EmissiveMult);
                rf::SendFloat(MetallicLoc, Mat.MetallicMult);
                rf::SendFloat(RoughnessLoc, Mat.RoughnessMult);
                rf::BindTexture2D(Mat.AlbedoTexture, 0);
                rf::BindTexture2D(Mat.NormalTexture, 1);
                rf::BindTexture2D(Mat.RoughnessMetallicTexture, 2);
                rf::BindTexture2D(Mat.EmissiveTexture, 3);
                rf::BindTexture2D(GGXLUT, 4);
                rf::BindCubemap(HDRGlossyEnvmap, 5);
                mat4f ModelMatrix;
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                ModelMatrix.FromTRS(translation[m]+vec3f(0,3,-3), vec3f(0,rotation[m],0), vec3f(1));
                ModelMatrix = ModelMatrix * Model->Mesh[i].ModelMatrix;
                rf::SendMat4(Loc, ModelMatrix);
                glBindVertexArray(Model->Mesh[i].VAO);
                rf::RenderMesh(&Model->Mesh[i]);
            }
        }
        rotation[0] += M_PI * Input->dTimeFixed * 0.02f;
        if(translation[0].y > 1.f) translationDir[0] = -1;
        if(translation[0].y < -1.f) translationDir[0] = 1;
        translation[0].y += Input->dTimeFixed * translationDir[0] * 0.05f;
    }
#endif

    int PBRCount = 3;
#if TEST_SPHEREARRAY // NOTE - Sphere Array Test for PBR
    {
        glUseProgram(Program3D);
        {
            uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
            rf::SendMat4(Loc, ViewMatrix);
            rf::CheckGLError("ViewMatrix");
        }
        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        rf::SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        rf::SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        rf::SendVec3(Loc, State->Camera.Position);

        glBindVertexArray(Sphere.VAO);
        mat4f ModelMatrix;
        Loc = glGetUniformLocation(Program3D, "ModelMatrix");

        rf::BindTexture2D(*Context->RenderResources.DefaultDiffuseTexture, 0);
        rf::BindTexture2D(*Context->RenderResources.DefaultNormalTexture, 1);
        rf::BindTexture2D(*Context->RenderResources.DefaultDiffuseTexture, 2);
        rf::BindTexture2D(*Context->RenderResources.DefaultEmissiveTexture, 3);
        rf::BindTexture2D(GGXLUT, 4);
        rf::BindCubemap(HDRGlossyEnvmap, 5);

        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
        rf::SendVec3(AlbedoLoc, vec3f(0.7, 0.7, 0.7));
        rf::SendVec3(EmissiveLoc, vec3f(1));
        for(int j = 0; j < PBRCount; ++j)
        {
            rf::SendFloat(MetallicLoc, (j)/(real32)PBRCount);
            for(int i = 0; i < 9; ++i)
            {
                rf::SendFloat(RoughnessLoc, Clamp((i)/9.0, 0.05f, 1.f));
                ModelMatrix.FromTRS(vec3f(-3*(j), 3.0, 3*(i+1)), vec3f(0.f), vec3f(1.f));
                rf::SendMat4(Loc, ModelMatrix);
                rf::RenderMesh(&Sphere);
            }
        }
        glActiveTexture(GL_TEXTURE0);
    }
#endif

#if TEST_PBRMATERIALS // NOTE - PBR materials rendering tests
    {
        glUseProgram(Program3D);
        {
            uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
            rf::SendMat4(Loc, ViewMatrix);
        }
        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        rf::SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        rf::SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        rf::SendVec3(Loc, State->Camera.Position);

        glBindVertexArray(Sphere.VAO);
        mat4f ModelMatrix;
        Loc = glGetUniformLocation(Program3D, "ModelMatrix");

        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");

        rf::BindTexture2D(GGXLUT, 4);
        rf::BindCubemap(HDRGlossyEnvmap, 5);
        rf::SendVec3(AlbedoLoc, vec3f(1));
        rf::SendVec3(EmissiveLoc, vec3f(1));
        rf::SendFloat(MetallicLoc, 1.0);

        auto DrawSpheres = [&](int idx, uint32 albedo, uint32 mr, uint32 nrm)
        {
            rf::BindTexture2D(albedo, 0);
            rf::BindTexture2D(nrm, 1);
            rf::BindTexture2D(mr, 2);
            rf::BindTexture2D(*Context->RenderResources.DefaultEmissiveTexture, 3);
            for(int i = 0; i < 9; ++ i)
            {
                rf::SendFloat(RoughnessLoc, 1.0 + 0.5 * i);
                ModelMatrix.FromTRS(vec3f(-(PBRCount+idx) * 3.0, 3.0, 3.0*(i+1)), vec3f(0.f), vec3f(1.f));
                rf::SendMat4(Loc, ModelMatrix);
                rf::RenderMesh(&Sphere);
            }
        };
        for(uint32 i = 0; i < MaterialCount; ++i)
        {
            DrawSpheres(i, *PBR_Albedo[i], *PBR_MetalRoughness[i], *PBR_Normal[i]);
        }

        glBindVertexArray(0);
        rf::CheckGLError("PBR draw");

        glActiveTexture(GL_TEXTURE0);
    }
#endif

#if TEST_PLANE // NOTE - CUBE DRAWING Test Put somewhere else
    {
        glUseProgram(Program3D);
        {

            uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
            rf::SendMat4(Loc, ViewMatrix);
            CheckGLError("ViewMatrix");
        }

        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        rf::SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        rf::SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        rf::SendVec3(Loc, State->Camera.Position);
        glBindVertexArray(Cube.VAO);
        mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
        Loc = glGetUniformLocation(Program3D, "ModelMatrix");
        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");

        rf::SendVec3(AlbedoLoc, vec3f(1));
        rf::SendVec3(EmissiveLoc, vec3f(1));
        rf::SendFloat(MetallicLoc, 0.1f);
        rf::SendFloat(RoughnessLoc, 0.9f);
        glBindVertexArray(Cube.VAO);
        rf::BindTexture2D(*Context->RenderResources.DefaultDiffuseTexture, 0);
        rf::BindTexture2D(*Context->RenderResources.DefaultNormalTexture, 1);
        rf::BindTexture2D(*Context->RenderResources.DefaultDiffuseTexture, 2);
        rf::BindTexture2D(*Context->RenderResources.DefaultEmissiveTexture, 3);
        rf::BindTexture2D(GGXLUT, 0);
        rf::BindCubemap(HDRGlossyEnvmap, 0);

        float planesize = 20;
        ModelMatrix.FromTRS(vec3f(-planesize/2.f-2,1,planesize/2.f+2), vec3f(0), vec3f(planesize,0.2f,planesize));
        rf::SendMat4(Loc, ModelMatrix);
        rf::CheckGLError("ModelMatrix");
        rf::RenderMesh(&Cube);
    }
#endif
#if TEST_SOUND // NOTE - Sound play
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

#if TEST_SKYBOX // NOTE - Skybox Rendering Test, put somewhere else
    { 
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        rf::CheckGLError("Skybox");

        glUseProgram(ProgramSkybox);
        {
            // NOTE - remove translation component from the ViewMatrix for the skybox
            mat4f SkyViewMatrix = ViewMatrix;
            SkyViewMatrix.SetTranslation(vec3f(0.f));
            uint32 Loc = glGetUniformLocation(ProgramSkybox, "ViewMatrix");
            rf::SendMat4(Loc, SkyViewMatrix);
            rf::CheckGLError("ViewMatrix Skybox");
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);
        glBindVertexArray(SkyboxCube.VAO);
        glDrawElements(GL_TRIANGLES, SkyboxCube.IndexCount, SkyboxCube.IndexType, 0);

        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
    }
#endif


}
}
