#ifndef RENDER_H
#define RENDER_H

#include "definitions.h"
#include "GL/glew.h"

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


void CheckGLError(const char *Mark = "");

image ResourceLoadImage(path const ExecutablePath, path const Filename, bool IsFloat, bool FlipY = true, 
                        int32 ForceNumChannel = 0);
void DestroyImage(image *Image);
font ResourceLoadFont(game_memory *Memory, path const Filename, real32 PixelHeight);
uint32 Make2DTexture(void *ImageBuffer, uint32 Width, uint32 Height, uint32 Channels, bool IsFloat, 
                     bool FloatHalfPrecision, real32 AnisotropicLevel, int MagFilter = GL_LINEAR, 
                     int MinFilter = GL_LINEAR_MIPMAP_LINEAR, int WrapS = GL_REPEAT, int WrapT = GL_REPEAT);
uint32 Make2DTexture(image *Image, bool IsFloat, bool FloatHalfPrecision, uint32 AnisotropicLevel);

uint32 MakeCubemap(path ExecutablePath, path *Paths, bool IsFloat, bool FloatHalfPrecision, uint32 Width, uint32 Height);
void ComputeIrradianceCubemap(game_memory *Memory, path ExecFullPath, char const *HDREnvmapFilename, 
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

bool ResourceLoadGLTFModel(model *Model, path const Filename, game_context *Context);
#endif
