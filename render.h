#ifndef RENDER_H
#define RENDER_H

#include "radar_common.h"

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
};

#endif
