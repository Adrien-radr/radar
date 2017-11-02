#ifndef RENDER_H
#define RENDER_H

#include "definitions.h"
#include "GL/glew.h"
#include <map>

struct game_context;

struct image
{
    void *Buffer;
    int32 Width;
    int32 Height;
    int32 Channels;
};

struct glyph
{
    // NOTE - Schema of behavior
    //   X,Y 0---------o   x
    //       |         |   |
    //       |         |   |
    //       |         |   | CH
    //       |         |   |
    //       0---------o   v
    //       x---------> CW
    //       x-----------> AdvX
    int X, Y;
    real32 TexX0, TexY0, TexX1, TexY1; // Absolute texcoords in Font Bitmap where the char is
    int CW, CH;
    real32 AdvX;
};

// NOTE - Ascii-only
// TODO - UTF
struct font
{
    uint8 *Buffer;
    int Width;
    int Height;
    int LineGap;
    int Ascent;
    real32 MaxGlyphWidth;
    glyph Glyphs[127-32]; // 126 is '~', 32 is ' '
    uint32 AtlasTextureID;
};

struct display_text
{
    uint32 VAO;
    uint32 VBO[2]; // 0 : positions+texcoords batched, 1 : indices
    uint32 IndexCount;
    uint32 Texture;
    vec4f  Color;
};

struct mesh
{
    uint32 VAO;
    uint32 VBO[3]; // 0: indices, 1-? : data
    uint32 IndexCount;
    uint32 IndexType;
};

struct material
{
    uint32 AlbedoTexture;
    uint32 RoughnessMetallicTexture;

    vec3f AlbedoMult;
    float RoughnessMult;
    float MetallicMult;
};

struct model
{
    std::vector<mesh> Mesh;
    std::vector<int> MaterialIdx;
    std::vector<material> Material;
};

enum render_resource_type
{
    RESOURCE_IMAGE,
    //RESOURCE_PROGRAM,
    RESOURCE_TEXTURE,
    RESOURCE_FONT,
    RESOURCE_COUNT
};

struct resource_store
{
    std::vector<char*> Keys;
    std::vector<void*> Values;
};

struct render_resources
{
    resource_helper *RH;
    uint32 *DefaultDiffuseTexture;
    uint32 *DefaultNormalTexture;

    resource_store Images;
    resource_store Textures;
    resource_store Fonts;
};

void CheckGLError(const char *Mark = "");

void *ResourceCheckExist(render_resources *RenderResources, render_resource_type Type, path const Filename);
void ResourceStore(render_resources *RenderResources, render_resource_type Type, path const Filename, void *Resource);
void ResourceFree(render_resources *RenderResources);
image *ResourceLoadImage(render_resources *RenderResources, path const Filename, bool IsFloat, bool FlipY = true,
                         int32 ForceNumChannel = 0);
void DestroyImage(image *Image);
font *ResourceLoadFont(render_resources *RenderResources, path const Filename, uint32 PixelHeight);
uint32 *ResourceLoad2DTexture(render_resources *RenderResources, path const Filename, bool IsFloat, bool FloatHalfPrecision,
                              uint32 AnisotropicLevel, int32 ForceNumChannel = 0);

uint32 MakeCubemap(render_resources *RenderResources, path *Paths, bool IsFloat, bool FloatHalfPrecision, uint32 Width, uint32 Height);
void ComputeIrradianceCubemap(render_resources *RenderResources, char const *HDREnvmapFilename,
        uint32 *HDRCubemapEnvmap, uint32 *HDRIrradianceEnvmap);

mesh MakeUnitCube(bool MakeAdditionalAttribs = true);
mesh Make2DQuad(vec2i Start, vec2i End);
mesh Make3DPlane(game_memory *Memory, vec2i Dimension, uint32 Subdivisions, uint32 TextureRepeatCount, bool Dynamic = false);
mesh MakeUnitSphere(bool MakeAdditionalAttribs = true);

uint32 BuildShader(game_memory *Memory, char *VSPath, char *FSPath);

void SendVec2(uint32 Loc, vec2f value);
void SendVec3(uint32 Loc, vec3f value);
void SendVec4(uint32 Loc, vec4f value);
void SendMat4(uint32 Loc, mat4f value);
void SendInt(uint32 Loc, int value);
void SendFloat(uint32 Loc, real32 value);

uint32 MakeVertexArrayObject();
uint32 AddIBO(uint32 Usage, uint32 Size, void const *Data);
uint32 AddEmptyVBO(uint32 Size, uint32 Usage);
void FillVBO(uint32 Attrib, uint32 AttribStride, uint32 Type, size_t ByteOffset, uint32 Size, void const *Data);
void UpdateVBO(uint32 VBO, size_t ByteOffset, uint32 Size, void *Data);
void DestroyMesh(mesh *Mesh);

void FillDisplayTextInterleaved(char const *Text, uint32 TextLength, font *Font, vec3i Pos, int MaxPixelWidth,
                                real32 *VertData, uint16 *Indices);

bool ResourceLoadGLTFModel(render_resources *RenderResources, model *Model, path const Filename, game_context *Context);
#endif
