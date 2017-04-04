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
    switch(Channels)
    {
    case 1:
        BaseFormat = Format = GL_RED; break;
    case 2:
        BaseFormat = Format = GL_RG; break;
    case 3:
        BaseFormat = Format = GL_RGB; break;
    case 4:
        BaseFormat = Format = GL_RGBA; break;
    }

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

// TODO - Load Unicode characters
// TODO - This method isn't perfect. Some letters have KERN advance between them when in sentences.
// This doesnt take it into account since we bake each letter separately for future use by texture lookup
font LoadFont(char *Filename, real32 PixelHeight)
{
    font Font = {};

    void *Contents = ReadFileContents(Filename);
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

    VSrc = (char*)ReadFileContents(VSPath);
    FSrc = (char*)ReadFileContents(FSPath);

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

uint32 AddVertexBufferObject(uint32 Attrib, uint32 AttribStride, uint32 Type, 
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

uint32 AddIndexBufferObject(uint32 Usage, uint32 Size, void *Data)
{
    uint32 Buffer;
    glGenBuffers(1, &Buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, Size, Data, Usage);

    return Buffer;
}

void DestroyMesh(mesh *Mesh)
{
    glDeleteBuffers(3, Mesh->VBO);
    glDeleteVertexArrays(1, &Mesh->VAO);
    Mesh->IndexCount = 0;
}

////////////////////////////////////////////////////////////////////////
// NOTE - Primitive builders
////////////////////////////////////////////////////////////////////////
mesh MakeUnitCube()
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
    Cube.VBO[0] = AddVertexBufferObject(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(Position), Position);
    Cube.VBO[1] = AddVertexBufferObject(1, 2, GL_FLOAT, GL_STATIC_DRAW, sizeof(Texcoord), Texcoord);
    Cube.VBO[2] = AddIndexBufferObject(GL_STATIC_DRAW, sizeof(Indices), Indices);
    glBindVertexArray(0);

    return Cube;
}




