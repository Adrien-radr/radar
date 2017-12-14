#define TEST_MODELS 1
#define TEST_SPHEREARRAY 1
#define TEST_PBRMATERIALS 1
#define TEST_PLANE 0

namespace Tests
{
// PBR Materials tests
uint32 GGXLUT = 0;
uint32 HDRCubemapEnvmap, HDRIrradianceEnvmap, HDRGlossyEnvmap;
mesh Sphere = {};
mesh Cube = {};

// PBR GLTF Models tests
uint32 const PBRModelsCount = 1;
uint32 const PBRModelsCapacity = 2;
model PBRModels[PBRModelsCapacity];
float rotation[PBRModelsCapacity] = { 0.f };
vec3f translation[PBRModelsCapacity] = { vec3f(-5.f, 0.f, 0.f) , vec3f(0,-1.98f,0) };
int translationDir[PBRModelsCapacity] = { 1 };

// PBR MATERIALS
uint32 *Texture1;
uint32 const MaterialCount = 6;
uint32 *PBR_Albedo[MaterialCount];
uint32 *PBR_MetalRoughness[MaterialCount];
uint32 *PBR_Normal[MaterialCount];

void Destroy()
{
    glDeleteTextures(1, &GGXLUT);
    glDeleteTextures(1, &HDRCubemapEnvmap);
    glDeleteTextures(1, &HDRGlossyEnvmap);
    glDeleteTextures(1, &HDRIrradianceEnvmap);
    DestroyMesh(&Sphere);
    DestroyMesh(&Cube);
}

bool Init(game_context *Context, game_config const &Config)
{
    GGXLUT = PrecomputeGGXLUT(&Context->RenderResources, 512);
    ComputeIrradianceCubemap(&Context->RenderResources, "data/envmap_monument.hdr", 
                                &HDRCubemapEnvmap, &HDRGlossyEnvmap, &HDRIrradianceEnvmap);
    Sphere = MakeUnitSphere(true, 3);
    Cube = MakeUnitCube();

#if TEST_MODELS
    if(!ResourceLoadGLTFModel(&Context->RenderResources, &PBRModels[0], "data/gltftest/suzanne/Suzanne.gltf", Context))
        return false;
    //if(!ResourceLoadGLTFModel(&Context->RenderResources, &PBRModels[1], "data/gltftest/lantern/Lantern.gltf", Context))
    //return 1;
#endif

#if TEST_PBRMATERIALS
    // PBR Texture Test
    Texture1 = ResourceLoad2DTexture(&Context->RenderResources, "data/crate1_diffuse.png",
            false, false, Config.AnisotropicFiltering);

    PBR_Albedo[0] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/StreakedMetal/albedo.png",
            false, false, Config.AnisotropicFiltering);
    PBR_MetalRoughness[0] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/StreakedMetal/metalroughness.png",
            false, false, Config.AnisotropicFiltering);
    PBR_Normal[0] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/StreakedMetal/normal.png",
            false, false, Config.AnisotropicFiltering);

    PBR_Albedo[1] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/ScuffedGold/albedo.png",
            false, false, Config.AnisotropicFiltering);
    PBR_MetalRoughness[1] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/ScuffedGold/metalroughness.png",
            false, false, Config.AnisotropicFiltering);
    PBR_Normal[1] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/ScuffedGold/normal.png",
            false, false, Config.AnisotropicFiltering);

    PBR_Albedo[2] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/RustedIron/albedo.png",
            false, false, Config.AnisotropicFiltering);
    PBR_MetalRoughness[2] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/RustedIron/metalroughness.png",
            false, false, Config.AnisotropicFiltering);
    PBR_Normal[2] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/RustedIron/normal.png",
            false, false, Config.AnisotropicFiltering);

    PBR_Albedo[3] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/ScuffedPlastic/albedo_blue.png",
            false, false, Config.AnisotropicFiltering);
    PBR_MetalRoughness[3] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/ScuffedPlastic/metalroughness.png",
            false, false, Config.AnisotropicFiltering);
    PBR_Normal[3] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/ScuffedPlastic/normal.png",
            false, false, Config.AnisotropicFiltering);

    PBR_Albedo[4] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/SynthRubber/albedo.png",
            false, false, Config.AnisotropicFiltering);
    PBR_MetalRoughness[4] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/SynthRubber/metalroughness.png",
            false, false, Config.AnisotropicFiltering);
    PBR_Normal[4] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/SynthRubber/normal.png",
            false, false, Config.AnisotropicFiltering);

    PBR_Albedo[5] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/PlasticPattern/albedo.png",
            false, false, Config.AnisotropicFiltering);
    PBR_MetalRoughness[5] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/PlasticPattern/metalroughness.png",
            false, false, Config.AnisotropicFiltering);
    PBR_Normal[5] = ResourceLoad2DTexture(&Context->RenderResources, "data/PBRTextures/PlasticPattern/normal.png",
            false, false, Config.AnisotropicFiltering);
#endif
    return true;
}

void Render(game_context *Context, game_state *State, game_input *Input, mat4f const &ViewMatrix)
{
#if TEST_MODELS // NOTE - Model rendering test
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
        SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        SendVec3(Loc, State->Camera.Position);
        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
        for(uint32  m = 0; m < PBRModelsCount; ++m)
        {
            model *Model = &PBRModels[m];
            for(uint32 i = 0; i < Model->Mesh.size(); ++i)
            {
                material const &Mat = Model->Material[Model->MaterialIdx[i]];

                SendVec3(AlbedoLoc, Mat.AlbedoMult);
                SendVec3(EmissiveLoc, Mat.EmissiveMult);
                SendFloat(MetallicLoc, Mat.MetallicMult);
                SendFloat(RoughnessLoc, Mat.RoughnessMult);
                glBindVertexArray(Model->Mesh[i].VAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Mat.AlbedoTexture);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Mat.NormalTexture);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, Mat.RoughnessMetallicTexture);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, Mat.EmissiveTexture);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, GGXLUT);
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);
                mat4f ModelMatrix;
                Loc = glGetUniformLocation(Program3D, "ModelMatrix");
                ModelMatrix.FromTRS(translation[m]+vec3f(0,3,-3), vec3f(0,rotation[m],0), vec3f(1));
                ModelMatrix = ModelMatrix * Model->Mesh[i].ModelMatrix;
                SendMat4(Loc, ModelMatrix);
                glDrawElements(GL_TRIANGLES, Model->Mesh[i].IndexCount, Model->Mesh[i].IndexType, 0);
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
            SendMat4(Loc, ViewMatrix);
            CheckGLError("ViewMatrix");
        }
        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        SendVec3(Loc, State->Camera.Position);

        glBindVertexArray(Sphere.VAO);
        mat4f ModelMatrix;
        Loc = glGetUniformLocation(Program3D, "ModelMatrix");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultDiffuseTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultNormalTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultDiffuseTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultEmissiveTexture);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, GGXLUT);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);

        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");
        SendVec3(AlbedoLoc, vec3f(0.7, 0.7, 0.7));
        SendVec3(EmissiveLoc, vec3f(1));
        for(int j = 0; j < PBRCount; ++j)
        {
            SendFloat(MetallicLoc, (j)/(real32)PBRCount);
            for(int i = 0; i < 9; ++i)
            {
                SendFloat(RoughnessLoc, Clamp((i)/9.0, 0.05f, 1.f));
                ModelMatrix.FromTRS(vec3f(-3*(j), 3.0, 3*(i+1)), vec3f(0.f), vec3f(1.f));
                SendMat4(Loc, ModelMatrix);
                glDrawElements(GL_TRIANGLES, Sphere.IndexCount, Sphere.IndexType, 0);
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
            SendMat4(Loc, ViewMatrix);
        }
        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        SendVec3(Loc, State->Camera.Position);

        glBindVertexArray(Sphere.VAO);
        mat4f ModelMatrix;
        Loc = glGetUniformLocation(Program3D, "ModelMatrix");

        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, GGXLUT);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);
        SendVec3(AlbedoLoc, vec3f(1));
        SendVec3(EmissiveLoc, vec3f(1));
        SendFloat(MetallicLoc, 1.0);

        auto DrawSpheres = [&](int idx, uint32 albedo, uint32 mr, uint32 nrm)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, albedo);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, nrm);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, mr);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultEmissiveTexture);
            for(int i = 0; i < 9; ++ i)
            {
                SendFloat(RoughnessLoc, 1.0 + 0.5 * i);
                ModelMatrix.FromTRS(vec3f(-(PBRCount+idx) * 3.0, 3.0, 3.0*(i+1)), vec3f(0.f), vec3f(1.f));
                SendMat4(Loc, ModelMatrix);
                glDrawElements(GL_TRIANGLES, Sphere.IndexCount, Sphere.IndexType, 0);
            }
        };
        for(int i = 0; i < MaterialCount; ++i)
        {
            DrawSpheres(i, *PBR_Albedo[i], *PBR_MetalRoughness[i], *PBR_Normal[i]);
        }

        CheckGLError("PBR draw");

        glActiveTexture(GL_TEXTURE0);
    }
#endif

#if TEST_PLANE // NOTE - CUBE DRAWING Test Put somewhere else
    {
        glUseProgram(Program3D);
        {

            uint32 Loc = glGetUniformLocation(Program3D, "ViewMatrix");
            SendMat4(Loc, ViewMatrix);
            CheckGLError("ViewMatrix");
        }

        uint32 Loc = glGetUniformLocation(Program3D, "LightColor");
        SendVec4(Loc, State->LightColor);
        Loc = glGetUniformLocation(Program3D, "SunDirection");
        SendVec3(Loc, State->SunDirection);
        Loc = glGetUniformLocation(Program3D, "CameraPos");
        SendVec3(Loc, State->Camera.Position);
        glBindVertexArray(Cube.VAO);
        mat4f ModelMatrix;// = mat4f::Translation(State->PlayerPosition);
        Loc = glGetUniformLocation(Program3D, "ModelMatrix");
        uint32 AlbedoLoc = glGetUniformLocation(Program3D, "AlbedoMult");
        uint32 EmissiveLoc = glGetUniformLocation(Program3D, "EmissiveMult");
        uint32 MetallicLoc = glGetUniformLocation(Program3D, "MetallicMult");
        uint32 RoughnessLoc = glGetUniformLocation(Program3D, "RoughnessMult");

        SendVec3(AlbedoLoc, vec3f(1));
        SendVec3(EmissiveLoc, vec3f(1));
        SendFloat(MetallicLoc, 0.1f);
        SendFloat(RoughnessLoc, 0.9f);
        glBindVertexArray(Cube.VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultDiffuseTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultDiffuseTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultNormalTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, *Context->RenderResources.DefaultEmissiveTexture);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, GGXLUT);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, HDRGlossyEnvmap);

        float planesize = 20;
        ModelMatrix.FromTRS(vec3f(-planesize/2.f-2,1,planesize/2.f+2), vec3f(0), vec3f(planesize,0.2f,planesize));
        SendMat4(Loc, ModelMatrix);
        CheckGLError("ModelMatrix");
        RenderMesh(&Cube);
    }
#endif
}
}
