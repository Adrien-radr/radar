#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_truetype.h"
#include "ext/stb_image.h"

#include "render.h"

void CheckGLError(const char *Mark = "")
{
    uint32 Err = glGetError();
    if(Err != GL_NO_ERROR)
    {
        char ErrName[32];
        switch(Err)
        {
            case GL_INVALID_ENUM:
                snprintf(ErrName, 32, "GL_INVALID_ENUM");
                break;
            case GL_INVALID_VALUE:
                snprintf(ErrName, 32, "GL_INVALID_VALUE");
                break;
            case GL_INVALID_OPERATION:
                snprintf(ErrName, 32, "GL_INVALID_OPERATION");
                break;
            default:
                snprintf(ErrName, 32, "UNKNOWN [%u]", Err);
                break;
        }
        printf("[%s] GL Error %s\n", Mark, ErrName);
    }
}

image LoadImage(char *Filename, bool FlipY = true, int32 ForceNumChannel = 0)
{
    image Image = {};
    stbi_set_flip_vertically_on_load(FlipY ? 1 : 0); // NOTE - Flip Y so textures are Y-descending
    Image.Buffer = stbi_load(Filename, &Image.Width, &Image.Height, &Image.Channels, ForceNumChannel);

    if(!Image.Buffer)
    {
        printf("Error loading Image from %s.\n", Filename);
    }

    return Image;
}

void DestroyImage(image *Image)
{
    stbi_image_free(Image->Buffer);
    Image->Width = Image->Height = Image->Channels = 0;
}

void FormatFromChannels(uint32 Channels, GLint *BaseFormat, GLint *Format)
{
    switch(Channels)
    {
    case 1:
        *BaseFormat = *Format = GL_RED; break;
    case 2:
        *BaseFormat = *Format = GL_RG; break;
    case 3:
        *BaseFormat = *Format = GL_RGB; break;
    case 4:
        *BaseFormat = *Format = GL_RGBA; break;
    }
}

uint32 Make2DTexture(uint8 *Bitmap, uint32 Width, uint32 Height, uint32 Channels, real32 AnisotropicLevel)
{
    uint32 Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    GLint CurrentAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &CurrentAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, AnisotropicLevel);

    GLint BaseFormat, Format;
    FormatFromChannels(Channels, &BaseFormat, &Format);

    glTexImage2D(GL_TEXTURE_2D, 0, BaseFormat, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Bitmap);
    glGenerateMipmap(GL_TEXTURE_2D);

    CheckGLError("glTexImage2D");
    glPixelStorei(GL_UNPACK_ALIGNMENT, CurrentAlignment);

    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture;
}

uint32 Make2DTexture(image *Image, uint32 AnisotropicLevel)
{
    return Make2DTexture(Image->Buffer, Image->Width, Image->Height, Image->Channels, AnisotropicLevel);
}

uint32 MakeCubemap(path *Paths)
{
    uint32 Cubemap = 0;

    glGenTextures(1, &Cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Cubemap);
    CheckGLError("SkyboxGen");

    // Load each face
    for(int i = 0; i < 6; ++i)
    {
        image Face = LoadImage(Paths[i]);

        GLint BaseFormat, Format;
        FormatFromChannels(Face.Channels, &BaseFormat, &Format);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, BaseFormat, Face.Width, Face.Height,
                0, Format, GL_UNSIGNED_BYTE, Face.Buffer);
        CheckGLError("SkyboxFace");

        DestroyImage(&Face);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    CheckGLError("SkyboxParams");

    return Cubemap;
}

// TODO - Load Unicode characters
// TODO - This method isn't perfect. Some letters have KERN advance between them when in sentences.
// This doesnt take it into account since we bake each letter separately for future use by texture lookup
font LoadFont(char *Filename, real32 PixelHeight)
{
    font Font = {};

    void *Contents = ReadFileContents(Filename, 0);
    if(Contents)
    {
        Font.Width = 1024;
        Font.Height = 1024;
        Font.Buffer = (uint8*)calloc(1, Font.Width*Font.Height);

        stbtt_fontinfo STBFont;
        stbtt_InitFont(&STBFont, (uint8*)Contents, 0);

        real32 PixelScale = stbtt_ScaleForPixelHeight(&STBFont, PixelHeight);
        int Ascent, Descent, LineGap;
        stbtt_GetFontVMetrics(&STBFont, &Ascent, &Descent, &LineGap);
        Ascent *= PixelScale;
        Descent *= PixelScale;

        Font.LineGap = Ascent - Descent;
        Font.Ascent = Ascent;


        int X = 0, Y = 0;

        for(int Codepoint = 32; Codepoint < 127; ++Codepoint)
        {
            int Glyph = stbtt_FindGlyphIndex(&STBFont, Codepoint);

            int AdvX;
            int X0, X1, Y0, Y1;
            stbtt_GetGlyphBitmapBox(&STBFont, Glyph, PixelScale, PixelScale, &X0, &Y0, &X1, &Y1);
            stbtt_GetGlyphHMetrics(&STBFont, Glyph, &AdvX, 0);
            //int AdvKern = stbtt_GetCodepointKernAdvance(&STBFont, Codepoint, Codepoint+1);

            int CW = X1 - X0;
            int CH = Y1 - Y0;
            real32 AdvanceX = (AdvX /*+ AdvKern*/) * PixelScale;

            if(X + CW >= Font.Width)
            {
                X = 0;
                Y += LineGap + Ascent + Descent;
                Assert((Y + Ascent - Descent) < Font.Height);
            }

             glyph TmpGlyph = { 
                X0, Y0,
                (X + X0)/(real32)Font.Width, (Y + Ascent + Y0)/(real32)Font.Height, 
                (X + X1)/(real32)Font.Width, (Y + Ascent + Y1)/(real32)Font.Height, 
                CW, CH, AdvanceX
            };
            Font.Glyphs[Codepoint-32] = TmpGlyph;

            int CharX = X + X0;
            int CharY = Y + Ascent + Y0;

            uint8 *BitmapPtr = Font.Buffer + (CharY * Font.Width + CharX);
            stbtt_MakeGlyphBitmap(&STBFont, BitmapPtr, CW, CH, Font.Width, PixelScale, PixelScale, Glyph);

            X += AdvanceX;
        }

        // Make Texture out of the Bitmap
        Font.AtlasTextureID = Make2DTexture(Font.Buffer, Font.Width, Font.Height, 1, 1.0f);

        FreeFileContents(Contents);
    }


    return Font;
}

void DestroyFont(font *Font)
{
    if(Font && Font->Buffer)
    {
        free(Font->Buffer);
    }
}

uint32 _CompileShader(char *Src, int Type)
{
    GLuint Shader = glCreateShader(Type);

    glShaderSource(Shader, 1, &Src, NULL);
    glCompileShader(Shader);

    GLint Status;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Status);

    if (!Status)
    {
        GLint Len;
        glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &Len);

        GLchar *Log = (GLchar*) malloc(Len);
        glGetShaderInfoLog(Shader, Len, NULL, Log);

        printf("Shader Compilation Error\n"
                "------------------------------------------\n"
                "%s"
                "------------------------------------------\n", Log);

        free(Log);
        Shader = 0;
    }
    return Shader;
}

uint32 BuildShader(char *VSPath, char *FSPath)
{
    char *VSrc = NULL, *FSrc = NULL;

    VSrc = (char*)ReadFileContents(VSPath, 0);
    FSrc = (char*)ReadFileContents(FSPath, 0);

    uint32 ProgramID = 0;
    bool IsValid = VSrc && FSrc;

    if(IsValid)
    {
        ProgramID = glCreateProgram(); 

        uint32 VShader = _CompileShader(VSrc, GL_VERTEX_SHADER);
        if(!VShader)
        {
            printf("Failed to build %s Vertex Shader.\n", VSPath);
            glDeleteProgram(ProgramID);
            return 0;
        }
        uint32 FShader = _CompileShader(FSrc, GL_FRAGMENT_SHADER);
        if(!VShader)
        {
            printf("Failed to build %s Vertex Shader.\n", VSPath);
            glDeleteShader(VShader);
            glDeleteProgram(ProgramID);
            return 0;
        }

        glAttachShader(ProgramID, VShader);
        glAttachShader(ProgramID, FShader);

        glDeleteShader(VShader);
        glDeleteShader(FShader);

        glLinkProgram(ProgramID);

        GLint Status;
        glGetProgramiv(ProgramID, GL_LINK_STATUS, &Status);

        if (!Status)
        {
            GLint Len;
            glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &Len);

            GLchar *Log = (GLchar*) malloc(Len);
            glGetProgramInfoLog(ProgramID, Len, NULL, Log);

            printf("Shader Program link error : \n"
                    "-----------------------------------------------------\n"
                    "%s"
                    "-----------------------------------------------------", Log);

            free(Log);
            glDeleteProgram(ProgramID);
            return 0;
        }
    }

    FreeFileContents(VSrc);
    FreeFileContents(FSrc);

    return ProgramID;
}

uint32 MakeVertexArrayObject()
{
    uint32 VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    return VAO;
}

uint32 AddEmptyVBO(uint32 Size, uint32 Usage)
{
    uint32 Buffer;
    glGenBuffers(1, &Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer);
    glBufferData(GL_ARRAY_BUFFER, Size, NULL, Usage);
    
    return Buffer;
}

void FillVBO(uint32 Attrib, uint32 AttribStride, uint32 Type,
             size_t ByteOffset, uint32 Size, void *Data)
{
    glEnableVertexAttribArray(Attrib);
    glBufferSubData(GL_ARRAY_BUFFER, ByteOffset, Size, Data);
    glVertexAttribPointer(Attrib, AttribStride, Type, GL_FALSE, 0, (GLvoid*)ByteOffset);
}

uint32 AddVBO(uint32 Attrib, uint32 AttribStride, uint32 Type, 
              uint32 Usage, uint32 Size, void *Data)
{
    glEnableVertexAttribArray(Attrib);

    uint32 Buffer;
    glGenBuffers(1, &Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer);
    glBufferData(GL_ARRAY_BUFFER, Size, Data, Usage);

    glVertexAttribPointer(Attrib, AttribStride, Type, GL_FALSE, 0, (GLvoid*)NULL);

    return Buffer;
}

uint32 AddIBO(uint32 Usage, uint32 Size, void *Data)
{
    uint32 Buffer;
    glGenBuffers(1, &Buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Size, Data, Usage);

    return Buffer;
}

void DestroyMesh(mesh *Mesh)
{
    glDeleteBuffers(2, Mesh->VBO);
    glDeleteVertexArrays(1, &Mesh->VAO);
    Mesh->IndexCount = 0;
}
display_text MakeDisplayText(font *Font, char const *Msg, int MaxPixelWidth, vec4f Color, real32 Scale = 1.0f)
{
    display_text Text = {};

    uint32 MsgLength = strlen(Msg);
    uint32 VertexCount = MsgLength * 4;
    uint32 IndexCount = MsgLength * 6;

    uint32 PS = 4 * 3;
    uint32 TS = 4 * 2;

    real32 *Positions = (real32*)alloca(3 * VertexCount * sizeof(real32));
    real32 *Texcoords = (real32*)alloca(2 * VertexCount * sizeof(real32));
    uint32 *Indices = (uint32*)alloca(IndexCount * sizeof(uint32));

    int X = 0, Y = 0;
    for(uint32 i = 0; i < MsgLength; ++i)
    {
        uint8 AsciiIdx = Msg[i] - 32; // 32 is the 1st Ascii idx
        glyph &Glyph = Font->Glyphs[AsciiIdx];

        // Modify DisplayWidth to always be at least the length of each character
        if(MaxPixelWidth < Glyph.CW)
        {
            MaxPixelWidth = Glyph.CW;
        }

        if(Msg[i] == '\n')
        {
            X = 0;
            Y -= Font->LineGap;
            AsciiIdx = Msg[++i] - 32;
            Glyph = Font->Glyphs[AsciiIdx];
            IndexCount -= 6;
        }

        if((X + Glyph.CW) >= MaxPixelWidth)
        {
            X = 0;
            Y -= Font->LineGap;
        }

        // Position (TL, BL, BR, TR)
        real32 BaseX = (real32)(X + Glyph.X);
        real32 BaseY = (real32)(Y - Font->Ascent - Glyph.Y);
        Positions[i*PS+0+0] = Scale*(BaseX);              Positions[i*PS+0+1] = Scale*(BaseY);            Positions[i*PS+0+2] = 0;
        Positions[i*PS+3+0] = Scale*(BaseX);              Positions[i*PS+3+1] = Scale*(BaseY - Glyph.CH); Positions[i*PS+3+2] = 0;
        Positions[i*PS+6+0] = Scale*(BaseX + Glyph.CW);   Positions[i*PS+6+1] = Scale*(BaseY - Glyph.CH); Positions[i*PS+6+2] = 0;
        Positions[i*PS+9+0] = Scale*(BaseX + Glyph.CW);   Positions[i*PS+9+1] = Scale*(BaseY);            Positions[i*PS+9+2] = 0;

        // texcoords
        Texcoords[i*TS+0+0] = Glyph.TexX0; Texcoords[i*TS+0+1] = Glyph.TexY0;
        Texcoords[i*TS+2+0] = Glyph.TexX0; Texcoords[i*TS+2+1] = Glyph.TexY1;
        Texcoords[i*TS+4+0] = Glyph.TexX1; Texcoords[i*TS+4+1] = Glyph.TexY1;
        Texcoords[i*TS+6+0] = Glyph.TexX1; Texcoords[i*TS+6+1] = Glyph.TexY0;

        Indices[i*6+0] = i*4+0;Indices[i*6+1] = i*4+1;Indices[i*6+2] = i*4+2;
        Indices[i*6+3] = i*4+0;Indices[i*6+4] = i*4+2;Indices[i*6+5] = i*4+3;

        X += Glyph.AdvX;
    }

    Text.VAO = MakeVertexArrayObject();
    Text.VBO[0] = AddEmptyVBO(5 * VertexCount * sizeof(real32), GL_STATIC_DRAW);
    FillVBO(0, 3, GL_FLOAT, 0, 3 * VertexCount * sizeof(real32), Positions);
    FillVBO(1, 2, GL_FLOAT, 3 * VertexCount * sizeof(real32), 2 * VertexCount * sizeof(real32), Texcoords);
    Text.VBO[1] = AddIBO(GL_STATIC_DRAW, IndexCount * sizeof(uint32), Indices);
    Text.IndexCount = IndexCount;
    glBindVertexArray(0);

    Text.Texture = Font->AtlasTextureID;
    Text.Color = Color;

    return Text;
}

void DestroyDisplayText(display_text *Text)
{
    glDeleteBuffers(2, Text->VBO);
    glDeleteVertexArrays(1, &Text->VAO);
    Text->IndexCount = 0;
}

////////////////////////////////////////////////////////////////////////
// NOTE - Primitive builders
////////////////////////////////////////////////////////////////////////
mesh MakeUnitCube(bool MakeTexcoord = true)
{
    mesh Cube = {};

    static vec3f Position[24] = {
        vec3f(-1, -1, -1),
        vec3f(-1, -1, 1),
        vec3f(-1, 1, 1),
        vec3f(-1, 1, -1),

        vec3f(1, -1, 1),
        vec3f(1, -1, -1),
        vec3f(1, 1, -1),
        vec3f(1, 1, 1),

        vec3f(-1, -1, 1),
        vec3f(-1, -1, -1),
        vec3f(1, -1, -1),
        vec3f(1, -1, 1),

        vec3f(-1, 1, -1),
        vec3f(-1, 1, 1),
        vec3f(1, 1, 1),
        vec3f(1, 1, -1),

        vec3f(1, 1, -1),
        vec3f(1, -1, -1),
        vec3f(-1, -1, -1),
        vec3f(-1, 1, -1),

        vec3f(-1, 1, 1),
        vec3f(-1, -1, 1),
        vec3f(1, -1, 1),
        vec3f(1, 1, 1)
    };

    static vec2f Texcoord[24] = {
        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),

        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),

        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),

        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),

        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),

        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),
    };

    uint32 Indices[36];
    for (uint32 i = 0; i < 6; ++i)
    {
        Indices[i * 6 + 0] = i * 4 + 0;
        Indices[i * 6 + 1] = i * 4 + 1;
        Indices[i * 6 + 2] = i * 4 + 2;

        Indices[i * 6 + 3] = i * 4 + 0;
        Indices[i * 6 + 4] = i * 4 + 2;
        Indices[i * 6 + 5] = i * 4 + 3;
    }

    Cube.IndexCount = 36;
    Cube.VAO = MakeVertexArrayObject();
    if(MakeTexcoord)
    {
        Cube.VBO[0] = AddEmptyVBO(sizeof(Position) + sizeof(Texcoord), GL_STATIC_DRAW);
        FillVBO(0, 3, GL_FLOAT, 0, sizeof(Position), Position);
        FillVBO(1, 2, GL_FLOAT, sizeof(Position), sizeof(Texcoord), Texcoord);
    }
    else
    {
        Cube.VBO[0] = AddVBO(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(Position), Position);
    }
    Cube.VBO[1] = AddIBO(GL_STATIC_DRAW, sizeof(Indices), Indices);
    glBindVertexArray(0);

    return Cube;
}

mesh MakeUnitSphere(bool MakeTexcoord = true)
{
    mesh Sphere = {};
    
    const real32 Radius = 1.f;
    const uint32 nLon = 32, nLat = 24;

    const uint32 nVerts = (nLon + 1) * nLat + 2;
    // const uint32 nTriangles = nVerts * 2;
    const uint32 nIndices = (nLat - 1)*nLon * 6 + nLon * 2 * 3;//nTriangles * 3;

    // Positions
    vec3f Position[nVerts];
    Position[0] = vec3f(0, 1, 0) * Radius;
    for (uint32 lat = 0; lat < nLat; ++lat)
    {
        real32 a1 = M_PI * (real32) (lat + 1) / (nLat + 1);
        real32 sin1 = sinf(a1);
        real32 cos1 = cosf(a1);

        for (uint32 lon = 0; lon <= nLon; ++lon)
        {
            real32 a2 = M_TWO_PI * (real32) (lon == nLon ? 0 : lon) / nLon;
            real32 sin2 = sinf(a2);
            real32 cos2 = cosf(a2);

            Position[lon + lat * (nLon + 1) + 1] = vec3f(sin1 * cos2, cos1, sin1 * sin2) * Radius;
        }
    }
    Position[nVerts - 1] = vec3f(0, 1, 0) * -Radius;

    // Normals
#if 0
    vec3f nrm[nVerts];
    for (uint32 i = 0; i < nVerts; ++i)
    {
        nrm[i] = Normalize(Position[i]);
    }
#endif

    // UVs
    vec2f Texcoord[nVerts];
    Texcoord[0] = vec2f(0, 1);
    Texcoord[nVerts - 1] = vec2f(0.f);
    for (uint32 lat = 0; lat < nLat; ++lat)
    {
        for (uint32 lon = 0; lon <= nLon; ++lon)
        {
            Texcoord[lon + lat * (nLon + 1) + 1] = vec2f(lon / (real32) nLon, 1.f - (lat + 1) / (real32) (nLat + 1));
        }
    }

    // Triangles/Indices
    uint32 Indices[nIndices];
    {
        // top
        uint32 i = 0;
        for (uint32 lon = 0; lon < nLon; ++lon)
        {
            Indices[i++] = lon + 2;
            Indices[i++] = lon + 1;
            Indices[i++] = 0;
        }

        // middle
        for (uint32 lat = 0; lat < nLat - 1; ++lat)
        {
            for (uint32 lon = 0; lon < nLon; ++lon)
            {
                uint32 curr = lon + lat * (nLon + 1) + 1;
                uint32 next = curr + nLon + 1;

                Indices[i++] = curr;
                Indices[i++] = curr + 1;
                Indices[i++] = next + 1;

                Indices[i++] = curr;
                Indices[i++] = next + 1;
                Indices[i++] = next;
            }
        }

        // bottom
        for (uint32 lon = 0; lon < nLon; ++lon)
        {
            Indices[i++] = nVerts - 1;
            Indices[i++] = nVerts - (lon + 2) - 1;
            Indices[i++] = nVerts - (lon + 1) - 1;
        }
    }

    Sphere.IndexCount = nIndices;
    Sphere.VAO = MakeVertexArrayObject();
    if(MakeTexcoord)
    {
        Sphere.VBO[0] = AddEmptyVBO(sizeof(Position) + sizeof(Texcoord), GL_STATIC_DRAW);
        FillVBO(0, 3, GL_FLOAT, 0, sizeof(Position), Position);
        FillVBO(1, 2, GL_FLOAT, sizeof(Position), sizeof(Texcoord), Texcoord);
    }
    else
    {
        Sphere.VBO[0] = AddVBO(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(Position), Position);
    }
    Sphere.VBO[1] = AddIBO(GL_STATIC_DRAW, sizeof(Indices), Indices);
    glBindVertexArray(0);

    return Sphere;
}



