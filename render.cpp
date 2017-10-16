#include "render.h"
#include "context.h"
#include "utils.h"

#include "stb_image.h"
#include "stb_truetype.h"

void CheckGLError(const char *Mark)
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

image ResourceLoadImage(path const ExecutablePath, path const Filename, bool IsFloat, bool FlipY, int32 ForceNumChannel)
{
    image Image = {};
    stbi_set_flip_vertically_on_load(FlipY ? 1 : 0); // NOTE - Flip Y so textures are Y-descending

    if(IsFloat)
        Image.Buffer = stbi_loadf(Filename, &Image.Width, &Image.Height, &Image.Channels, ForceNumChannel);
    else
        Image.Buffer = stbi_load(Filename, &Image.Width, &Image.Height, &Image.Channels, ForceNumChannel);

    if(!Image.Buffer)
    {
        printf("Error loading Image from %s. Loading default white texture.\n", Filename);
        path DefaultPath;
        MakeRelativePath(DefaultPath, ExecutablePath, "data/default_diffuse.png");
        if(IsFloat)
            Image.Buffer = stbi_loadf(DefaultPath, &Image.Width, &Image.Height, &Image.Channels, ForceNumChannel);
        else
            Image.Buffer = stbi_load(Filename, &Image.Width, &Image.Height, &Image.Channels, ForceNumChannel);

        if(!Image.Buffer)
        {
            printf("Can't find default white texture (data/default_diffuse.png). Aborting... \n");
            exit(1);
        }
            
    }

    return Image;
}

void DestroyImage(image *Image)
{
    stbi_image_free(Image->Buffer);
    Image->Width = Image->Height = Image->Channels = 0;
}

void FormatFromChannels(uint32 Channels, bool IsFloat, bool FloatHalfPrecision, GLint *BaseFormat, GLint *Format)
{
    if(IsFloat)
    {
        switch(Channels)
        {
            case 1:
                *BaseFormat = FloatHalfPrecision ? GL_R16F : GL_R32F;
                *Format = GL_RED; 
                break;
            case 2:
                *BaseFormat = FloatHalfPrecision ? GL_RG16F : GL_RG32F;
                *Format = GL_RG; 
                break;
            case 3:
                *BaseFormat = FloatHalfPrecision ? GL_RGB16F : GL_RGB32F;
                *Format = GL_RGB; 
                break;
            case 4:
                *BaseFormat = FloatHalfPrecision ? GL_RGBA16F : GL_RGBA32F;
                *Format = GL_RGBA; 
                break;
        }
    }
    else
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
}

uint32 Make2DTexture(void *ImageBuffer, uint32 Width, uint32 Height, uint32 Channels, bool IsFloat, 
        bool FloatHalfPrecision, real32 AnisotropicLevel, int MagFilter, int MinFilter, int WrapS, int WrapT)
{
    uint32 Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    GLint CurrentAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &CurrentAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, AnisotropicLevel);

    GLint BaseFormat, Format;
    FormatFromChannels(Channels, IsFloat, FloatHalfPrecision, &BaseFormat, &Format);
    GLenum Type = IsFloat ? GL_FLOAT : GL_UNSIGNED_BYTE;

    glTexImage2D(GL_TEXTURE_2D, 0, BaseFormat, Width, Height, 0, Format, Type, ImageBuffer);
    CheckGLError("glTexImage2D");
    if(MinFilter >= GL_NEAREST_MIPMAP_NEAREST && MinFilter <= GL_LINEAR_MIPMAP_LINEAR)
    { // NOTE - Generate mipmaps if mag filter ask it
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, CurrentAlignment);

    glBindTexture(GL_TEXTURE_2D, 0);

    return Texture;
}

uint32 Make2DTexture(image *Image, bool IsFloat, bool FloatHalfPrecision, uint32 AnisotropicLevel)
{
    return Make2DTexture(Image->Buffer, Image->Width, Image->Height, Image->Channels, IsFloat, FloatHalfPrecision, AnisotropicLevel);
}

uint32 MakeCubemap(path ExecutablePath, path *Paths, bool IsFloat, bool FloatHalfPrecision, uint32 Width, uint32 Height)
{
    uint32 Cubemap = 0;

    glGenTextures(1, &Cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Cubemap);
    CheckGLError("SkyboxGen");
    
    for(int i = 0; i < 6; ++i)
    { // Load each face
        if(Paths)
        { // For loading from texture files
            image Face = ResourceLoadImage(ExecutablePath, Paths[i], IsFloat);

            GLint BaseFormat, Format;
            FormatFromChannels(Face.Channels, IsFloat, FloatHalfPrecision, &BaseFormat, &Format);
            GLenum Type = IsFloat ? GL_FLOAT : GL_UNSIGNED_BYTE;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, BaseFormat, Face.Width, Face.Height,
                    0, Format, Type, Face.Buffer);
            CheckGLError("SkyboxFace");

            DestroyImage(&Face);
        }
        else
        { // Empty Cubemap
            GLint BaseFormat = IsFloat ? (FloatHalfPrecision ? GL_RGB16F : GL_RGB32F) : GL_RGB16;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, BaseFormat, 
                    Width, Height, 0, GL_RGB, IsFloat ? GL_FLOAT : GL_UNSIGNED_BYTE, NULL);
        }
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

#define MAX_FBO_ATTACHMENTS 5
static GLuint FBOAttachments[MAX_FBO_ATTACHMENTS] = 
{ 
    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 
};

struct frame_buffer
{
    vec2i Size;
    uint32 NumAttachments;
    uint32 FBO;
    uint32 DepthBufferID;
    uint32 BufferIDs[MAX_FBO_ATTACHMENTS];
};

void DestroyFramebuffer(frame_buffer *FB)
{
    glDeleteTextures(MAX_FBO_ATTACHMENTS, FB->BufferIDs);
    glDeleteRenderbuffers(1, &FB->DepthBufferID);
    glDeleteFramebuffers(1, &FB->FBO);
    FB->Size = vec2i(0);
    FB->FBO = 0;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

frame_buffer MakeFramebuffer(uint32 NumAttachments, vec2i Size)
{
    Assert(NumAttachments <= MAX_FBO_ATTACHMENTS);

    frame_buffer FB = {};
    glGenFramebuffers(1, &FB.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FB.FBO);
    glDrawBuffers((GLsizei) NumAttachments, FBOAttachments);

    // NOTE - Always attach a depth buffer : is there an instance where you dont want that ?
    glGenRenderbuffers(1, &FB.DepthBufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, FB.DepthBufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Size.x, Size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, FB.DepthBufferID);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer creation error : not complete.\n");
        DestroyFramebuffer(&FB);
    }

    FB.Size = Size;
    FB.NumAttachments = NumAttachments;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return FB;
}

void AttachBuffer(frame_buffer *FBO, uint32 Attachment, uint32 Channels, bool IsFloat, bool FloatHalfPrecision)
{
    Assert(Attachment < MAX_FBO_ATTACHMENTS);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO->FBO);

    uint32 *BufferID = &FBO->BufferIDs[Attachment];
    glGenTextures(1, BufferID);
    glBindTexture(GL_TEXTURE_2D, *BufferID);

    GLint BaseFormat, Format;
    FormatFromChannels(Channels, IsFloat, FloatHalfPrecision, &BaseFormat, &Format);
    GLenum Type = IsFloat ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glTexImage2D(GL_TEXTURE_2D, 0, BaseFormat, FBO->Size.x, FBO->Size.y, 0, Format, Type, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum) ( GL_COLOR_ATTACHMENT0 + Attachment ), GL_TEXTURE_2D, *BufferID, 0 );

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer creation error : Attachment %u error.\n", Attachment);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// TODO - Load Unicode characters
// TODO - This method isn't perfect. Some letters have KERN advance between them when in sentences.
// This doesnt take it into account since we bake each letter separately for future use by texture lookup
font ResourceLoadFont(game_memory *Memory, char const *Filename, real32 PixelHeight)
{
    font Font = {};

    void *Contents = ReadFileContents(&Memory->ScratchArena, Filename, 0);
    if(Contents)
    {
        Font.Width = 1024;
        Font.Height = 1024;
        Font.Buffer = (uint8*)PushArenaData(&Memory->SessionArena, Font.Width*Font.Height);

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
        Font.AtlasTextureID = Make2DTexture(Font.Buffer, Font.Width, Font.Height, 1, false, false, 1.0f);
    }


    return Font;
}

uint32 _CompileShader(game_memory *Memory, char *Src, int Type)
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

        GLchar *Log = (GLchar*) PushArenaData(&Memory->ScratchArena, Len);
        glGetShaderInfoLog(Shader, Len, NULL, Log);

        printf("Shader Compilation Error\n"
                "------------------------------------------\n"
                "%s"
                "------------------------------------------\n", Log);

        Shader = 0;
    }
    return Shader;
}

uint32 BuildShader(game_memory *Memory, char *VSPath, char *FSPath)
{
    char *VSrc = NULL, *FSrc = NULL;

    VSrc = (char*)ReadFileContents(&Memory->ScratchArena, VSPath, 0);
    FSrc = (char*)ReadFileContents(&Memory->ScratchArena, FSPath, 0);

    uint32 ProgramID = 0;
    bool IsValid = VSrc && FSrc;

    if(IsValid)
    {
        ProgramID = glCreateProgram(); 

        uint32 VShader = _CompileShader(Memory, VSrc, GL_VERTEX_SHADER);
        if(!VShader)
        {
            printf("Failed to build %s Vertex Shader.\n", VSPath);
            glDeleteProgram(ProgramID);
            return 0;
        }
        uint32 FShader = _CompileShader(Memory, FSrc, GL_FRAGMENT_SHADER);
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

            GLchar *Log = (GLchar*) PushArenaData(&Memory->ScratchArena, Len);
            glGetProgramInfoLog(ProgramID, Len, NULL, Log);

            printf("Shader Program link error : \n"
                    "-----------------------------------------------------\n"
                    "%s"
                    "-----------------------------------------------------", Log);

            glDeleteProgram(ProgramID);
            return 0;
        }
    }

    return ProgramID;
}

void SendVec2(uint32 Loc, vec2f value)
{
    glUniform2fv(Loc, 1, (GLfloat const *) value);
}

void SendVec3(uint32 Loc, vec3f value)
{
    glUniform3fv(Loc, 1, (GLfloat const *) value);
}

void SendVec4(uint32 Loc, vec4f value)
{
    glUniform4fv(Loc, 1, (GLfloat const *) value);
}

void SendMat4(uint32 Loc, mat4f value)
{
    glUniformMatrix4fv(Loc, 1, GL_FALSE, (GLfloat const *) value);
}

void SendInt(uint32 Loc, int value)
{
    glUniform1i(Loc, value);
}

void SendFloat(uint32 Loc, real32 value)
{
    glUniform1f(Loc, value);
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
             size_t ByteOffset, uint32 Size, void const *Data)
{
    glEnableVertexAttribArray(Attrib);
    CheckGLError("VA");
    glBufferSubData(GL_ARRAY_BUFFER, ByteOffset, Size, Data);
    CheckGLError("SB");
    glVertexAttribPointer(Attrib, AttribStride, Type, GL_FALSE, 0, (GLvoid*)ByteOffset);
    CheckGLError("VAP");
}

uint32 AddVBO(uint32 Attrib, uint32 AttribStride, uint32 Type, 
              uint32 Usage, uint32 Size, void const *Data)
{
    glEnableVertexAttribArray(Attrib);

    uint32 Buffer;
    glGenBuffers(1, &Buffer);
    glBindBuffer(GL_ARRAY_BUFFER, Buffer);
    glBufferData(GL_ARRAY_BUFFER, Size, Data, Usage);

    glVertexAttribPointer(Attrib, AttribStride, Type, GL_FALSE, 0, (GLvoid*)NULL);

    return Buffer;
}

void UpdateVBO(uint32 VBO, size_t ByteOffset, uint32 Size, void *Data)
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, ByteOffset, Size, Data);
}

uint32 AddIBO(uint32 Usage, uint32 Size, void const *Data)
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


void FillDisplayTextInterleaved(char const *Text, uint32 TextLength, font *Font, vec3i Pos, int MaxPixelWidth, 
                                real32 *VertData, uint16 *Indices)
{
    uint32 VertexCount = TextLength * 4;
    uint32 IndexCount = TextLength * 6;

    uint32 Stride = 5 * 4;

    int X = 0, Y = 0;
    for(uint32 i = 0; i < TextLength; ++i)
    {
        uint8 AsciiIdx = Text[i] - 32; // 32 is the 1st Ascii idx
        glyph &Glyph = Font->Glyphs[AsciiIdx];

        // Modify DisplayWidth to always be at least the length of each character
        if(MaxPixelWidth < Glyph.CW)
        {
            MaxPixelWidth = Glyph.CW;
        }

        if(Text[i] == '\n')
        {
            X = 0;
            Y -= Font->LineGap;
            AsciiIdx = Text[++i] - 32;
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
        vec3f TL = Pos + vec3f(BaseX, BaseY, 0);
        vec3f BR = TL  + vec3f(Glyph.CW, -Glyph.CH, 0);
        VertData[i*Stride+0+0] = TL.x;         VertData[i*Stride+0+1] = TL.y;        VertData[i*Stride+0+2] = TL.z;
        VertData[i*Stride+3+0] = Glyph.TexX0;  VertData[i*Stride+3+1] = Glyph.TexY0;
        VertData[i*Stride+5+0] = TL.x;         VertData[i*Stride+5+1] = BR.y;        VertData[i*Stride+5+2] = TL.z;
        VertData[i*Stride+8+0] = Glyph.TexX0;  VertData[i*Stride+8+1] = Glyph.TexY1;
        VertData[i*Stride+10+0] = BR.x;        VertData[i*Stride+10+1] = BR.y;       VertData[i*Stride+10+2] = TL.z;
        VertData[i*Stride+13+0] = Glyph.TexX1; VertData[i*Stride+13+1] = Glyph.TexY1;
        VertData[i*Stride+15+0] = BR.x;        VertData[i*Stride+15+1] = TL.y;       VertData[i*Stride+15+2] = TL.z;
        VertData[i*Stride+18+0] = Glyph.TexX1; VertData[i*Stride+18+1] = Glyph.TexY0;

        Indices[i*6+0] = i*4+0;Indices[i*6+1] = i*4+1;Indices[i*6+2] = i*4+2;
        Indices[i*6+3] = i*4+0;Indices[i*6+4] = i*4+2;Indices[i*6+5] = i*4+3;

        X += Glyph.AdvX;
    }

}
display_text MakeDisplayText(game_memory *Memory, font *Font, char const *Msg, int MaxPixelWidth, vec4f Color, real32 Scale = 1.0f)
{
    display_text Text = {};

    uint32 MsgLength = strlen(Msg);
    uint32 VertexCount = MsgLength * 4;
    uint32 IndexCount = MsgLength * 6;

    uint32 PS = 4 * 3;
    uint32 TS = 4 * 2;

    real32 *Positions = (real32*)PushArenaData(&Memory->ScratchArena, 3 * VertexCount * sizeof(real32));
    real32 *Texcoords = (real32*)PushArenaData(&Memory->ScratchArena, 2 * VertexCount * sizeof(real32));
    uint32 *Indices = (uint32*)PushArenaData(&Memory->ScratchArena, IndexCount * sizeof(uint32));

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
    Text.VBO[0] = AddIBO(GL_STATIC_DRAW, IndexCount * sizeof(uint32), Indices);
    Text.VBO[1] = AddEmptyVBO(5 * VertexCount * sizeof(real32), GL_STATIC_DRAW);
    FillVBO(0, 3, GL_FLOAT, 0, 3 * VertexCount * sizeof(real32), Positions);
    FillVBO(1, 2, GL_FLOAT, 3 * VertexCount * sizeof(real32), 2 * VertexCount * sizeof(real32), Texcoords);
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
mesh MakeUnitCube(bool MakeAdditionalAttribs)
{
    mesh Cube = {};

    vec3f const Position[24] = {
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

    vec2f const Texcoord[24] = {
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

    vec3f const Normal[24] = {
        vec3f(-1, 0, 0),
        vec3f(-1, 0, 0),
        vec3f(-1, 0, 0),
        vec3f(-1, 0, 0),

        vec3f(1, 0, 0),
        vec3f(1, 0, 0),
        vec3f(1, 0, 0),
        vec3f(1, 0, 0),

        vec3f(0, -1, 0),
        vec3f(0, -1, 0),
        vec3f(0, -1, 0),
        vec3f(0, -1, 0),

        vec3f(0, 1, 0),
        vec3f(0, 1, 0),
        vec3f(0, 1, 0),
        vec3f(0, 1, 0),

        vec3f(0, 0, -1),
        vec3f(0, 0, -1),
        vec3f(0, 0, -1),
        vec3f(0, 0, -1),

        vec3f(0, 0, 1),
        vec3f(0, 0, 1),
        vec3f(0, 0, 1),
        vec3f(0, 0, 1)
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
    Cube.VBO[0] = AddIBO(GL_STATIC_DRAW, sizeof(Indices), Indices);
    if(MakeAdditionalAttribs)
    {
        Cube.VBO[1] = AddEmptyVBO(sizeof(Position) + sizeof(Texcoord) + sizeof(Normal), GL_STATIC_DRAW);
        FillVBO(0, 3, GL_FLOAT, 0, sizeof(Position), Position);
        FillVBO(1, 2, GL_FLOAT, sizeof(Position), sizeof(Texcoord), Texcoord);
        FillVBO(2, 3, GL_FLOAT, sizeof(Position) + sizeof(Texcoord), sizeof(Normal), Normal);
    }
    else
    {
        Cube.VBO[1] = AddVBO(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(Position), Position);
    }
    glBindVertexArray(0);

    return Cube;
}

// NOTE - Start is Top Left corner. End is Bottom Right corner.
mesh Make2DQuad(vec2i Start, vec2i End)
{
    mesh Quad = {};

    vec2f Position[4] =
    {
        vec2f(Start.x, Start.y),
        vec2f(Start.x, End.y),
        vec2f(End.x, End.y),
        vec2f(End.x, Start.y)
    };

    vec2f Texcoord[4] =
    {
        vec2f(0, 1),
        vec2f(0, 0),
        vec2f(1, 0),
        vec2f(1, 1),
    };

    uint32 Indices[6] = { 0, 1, 2, 0, 2, 3 };

    Quad.IndexCount = 6;
    Quad.VAO = MakeVertexArrayObject();
    Quad.VBO[0] = AddIBO(GL_STATIC_DRAW, sizeof(Indices), Indices);
    Quad.VBO[1] = AddEmptyVBO(sizeof(Position) + sizeof(Texcoord), GL_STATIC_DRAW);
    FillVBO(0, 2, GL_FLOAT, 0, sizeof(Position), Position);
    FillVBO(1, 2, GL_FLOAT, sizeof(Position), sizeof(Texcoord), Texcoord);
    glBindVertexArray(0);
    
    return Quad;
}

mesh Make3DPlane(game_memory *Memory, vec2i Dimension, uint32 Subdivisions, uint32 TextureRepeatCount, bool Dynamic)
{
    mesh Plane = {};

    size_t BaseSize = Square(Subdivisions);
    size_t PositionsSize = 4 * BaseSize * sizeof(vec3f);
    size_t NormalsSize = 4 * BaseSize * sizeof(vec3f);
    size_t TexcoordsSize = 4 * BaseSize * sizeof(vec2f);
    size_t IndicesSize = 6 * BaseSize * sizeof(uint32);

    vec3f *Positions = (vec3f*)PushArenaData(&Memory->ScratchArena, PositionsSize);
    vec3f *Normals = (vec3f*)PushArenaData(&Memory->ScratchArena, NormalsSize);
    vec2f *Texcoords = (vec2f*)PushArenaData(&Memory->ScratchArena, TexcoordsSize);
    uint32 *Indices = (uint32*)PushArenaData(&Memory->ScratchArena, IndicesSize);

    vec2i SubdivDim = Dimension / Subdivisions;
    vec2f TexMax = Dimension / (real32)TextureRepeatCount;

    for(uint32 j = 0; j < Subdivisions; ++j)
    {
        for(uint32 i = 0; i < Subdivisions; ++i)
        {
            uint32 Idx = j*Subdivisions+i;

            Positions[Idx*4+0] = vec3f(i*SubdivDim.x, 0.f, j*SubdivDim.y);
            Positions[Idx*4+1] = vec3f(i*SubdivDim.x, 0.f, (j+1)*SubdivDim.y);
            Positions[Idx*4+2] = vec3f((i+1)*SubdivDim.x, 0.f, (j+1)*SubdivDim.y);
            Positions[Idx*4+3] = vec3f((i+1)*SubdivDim.x, 0.f, j*SubdivDim.y);

            Normals[Idx*4+0] = vec3f(0,1,0);
            Normals[Idx*4+1] = vec3f(0,1,0);
            Normals[Idx*4+2] = vec3f(0,1,0);
            Normals[Idx*4+3] = vec3f(0,1,0);

            Texcoords[Idx*4+0] = vec2f(0.f, TexMax.y);
            Texcoords[Idx*4+1] = vec2f(0.f, 0.f);
            Texcoords[Idx*4+2] = vec2f(TexMax.x, 0.f);
            Texcoords[Idx*4+3] = vec2f(TexMax.x, TexMax.y);

            Indices[Idx*6+0] = Idx*4+0;
            Indices[Idx*6+1] = Idx*4+1;
            Indices[Idx*6+2] = Idx*4+2;
            Indices[Idx*6+3] = Idx*4+0;
            Indices[Idx*6+4] = Idx*4+2;
            Indices[Idx*6+5] = Idx*4+3;
        }
    }

    Plane.IndexCount = 6 * BaseSize;
    Plane.VAO = MakeVertexArrayObject();
    Plane.VBO[0] = AddIBO(GL_STATIC_DRAW, IndicesSize, Indices);
    // Positions and Normals in the 1st VBO
    Plane.VBO[1] = AddEmptyVBO(PositionsSize + NormalsSize, Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    FillVBO(0, 3, GL_FLOAT, 0, PositionsSize, Positions);
    FillVBO(2, 3, GL_FLOAT, PositionsSize, NormalsSize, Normals);
    // Texcoords in the 2nd VBO
    Plane.VBO[2] = AddVBO(1, 2, GL_FLOAT, GL_STATIC_DRAW, TexcoordsSize, Texcoords);
    glBindVertexArray(0);

    return Plane;
}

mesh MakeUnitSphere(bool MakeAdditionalAttribs)
{
    mesh Sphere = {};
    
    real32 const Radius = 1.f;
    uint32 const nLon = 32, nLat = 24;

    uint32 const nVerts = (nLon + 1) * nLat + 2;
    uint32 const nIndices = (nLat - 1)*nLon * 6 + nLon * 2 * 3;

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
    vec3f Normal[nVerts];
    for (uint32 i = 0; i < nVerts; ++i)
    {
        Normal[i] = Normalize(Position[i]);
    }

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
    Sphere.VBO[0] = AddIBO(GL_STATIC_DRAW, sizeof(Indices), Indices);
    if(MakeAdditionalAttribs)
    {
        Sphere.VBO[1] = AddEmptyVBO(sizeof(Position) + sizeof(Texcoord) + sizeof(Normal), GL_STATIC_DRAW);
        FillVBO(0, 3, GL_FLOAT, 0, sizeof(Position), Position);
        FillVBO(1, 2, GL_FLOAT, sizeof(Position), sizeof(Texcoord), Texcoord);
        FillVBO(2, 3, GL_FLOAT, sizeof(Position) + sizeof(Texcoord), sizeof(Normal), Normal);
    }
    else
    {
        Sphere.VBO[1] = AddVBO(0, 3, GL_FLOAT, GL_STATIC_DRAW, sizeof(Position), Position);
    }
    glBindVertexArray(0);

    return Sphere;
}

void ComputeIrradianceCubemap(game_memory *Memory, path ExecFullPath, char const *HDREnvmapFilename, 
        uint32 *HDRCubemapEnvmap, uint32 *HDRIrradianceEnvmap)
{
    // TODO - Parameterize this ?
    uint32 CubemapWidth = 512;
    uint32 IrradianceCubemapWidth = 32;

    uint32 ProgramLatlong2Cubemap;
    uint32 ProgramCubemapConvolution;

    path VSPath;
    path FSPath;
    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/skybox_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/latlong2cubemap_frag.glsl");
    ProgramLatlong2Cubemap = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramLatlong2Cubemap);
    SendInt(glGetUniformLocation(ProgramLatlong2Cubemap, "Envmap"), 0);
    CheckGLError("Latlong Shader");

    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/skybox_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/cubemapconvolution_frag.glsl");
    ProgramCubemapConvolution = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramCubemapConvolution);
    SendInt(glGetUniformLocation(ProgramCubemapConvolution, "Cubemap"), 0);
    CheckGLError("Convolution Shader");

    frame_buffer FBOEnvmap = MakeFramebuffer(1, vec2i(CubemapWidth, CubemapWidth));

    path HDREnvmapImagePath;
    MakeRelativePath(HDREnvmapImagePath, ExecFullPath, HDREnvmapFilename);
    image HDREnvmapImage = ResourceLoadImage(Memory->ExecutableFullPath, HDREnvmapImagePath, true);

    uint32 HDRLatlongEnvmap = Make2DTexture(HDREnvmapImage.Buffer, HDREnvmapImage.Width, HDREnvmapImage.Height,
            HDREnvmapImage.Channels, true, false, 1);
    DestroyImage(&HDREnvmapImage);

    mesh SkyboxCube = MakeUnitCube(false);

    *HDRCubemapEnvmap = MakeCubemap(NULL, NULL, true, true, CubemapWidth, CubemapWidth);
    CheckGLError("Latlong2Cubmap");
    *HDRIrradianceEnvmap = MakeCubemap(NULL, NULL, true, false, IrradianceCubemapWidth, IrradianceCubemapWidth);
    CheckGLError("IrradianceCubemap");

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(SkyboxCube.VAO);
    // The 6 view matrices for the 6 cubemap directions
    mat4f const ViewDirs [6] = {
        mat4f::LookAt(vec3f(0), vec3f( 1, 0, 0), vec3f(0, -1, 0)),
        mat4f::LookAt(vec3f(0), vec3f(-1, 0, 0), vec3f(0, -1, 0)),
        mat4f::LookAt(vec3f(0), vec3f( 0, 1, 0), vec3f(0, 0, 1)),
        mat4f::LookAt(vec3f(0), vec3f( 0,-1, 0), vec3f(0, 0,-1)),
        mat4f::LookAt(vec3f(0), vec3f( 0, 0, 1), vec3f(0, -1, 0)),
        mat4f::LookAt(vec3f(0), vec3f( 0, 0,-1), vec3f(0, -1, 0))
    };
    mat4f const EnvmapProjectionMatrix = mat4f::Perspective(90.f, 1.f, 0.1f, 10.f);

    // NOTE - Latlong to Cubemap
    glUseProgram(ProgramLatlong2Cubemap);
    {
        uint32 Loc = glGetUniformLocation(ProgramLatlong2Cubemap, "ProjMatrix");
        SendMat4(Loc, EnvmapProjectionMatrix);
        CheckGLError("ProjMatrix Latlong2Cubemap");
    }

    uint32 ViewLoc = glGetUniformLocation(ProgramLatlong2Cubemap, "ViewMatrix");

    glViewport(0, 0, CubemapWidth, CubemapWidth);
    glBindTexture(GL_TEXTURE_2D, HDRLatlongEnvmap);
    glBindFramebuffer(GL_FRAMEBUFFER, FBOEnvmap.FBO);
    for(int i = 0; i < 6; ++i)
    {
        SendMat4(ViewLoc, ViewDirs[i]);
        CheckGLError("ViewMatrix Latlong2Cubemap");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                *HDRCubemapEnvmap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, SkyboxCube.IndexCount, GL_UNSIGNED_INT, 0);
    }

    // NOTE - Cubemap convolution
    glUseProgram(ProgramCubemapConvolution);
    {
        uint32 Loc = glGetUniformLocation(ProgramCubemapConvolution, "ProjMatrix");
        mat4f EnvmapProjectionMatrix = mat4f::Perspective(90.f, 1.f, 0.1f, 10.f);
        SendMat4(Loc, EnvmapProjectionMatrix);
        CheckGLError("ProjMatrix CubemapConvolution");
    }

    ViewLoc = glGetUniformLocation(ProgramCubemapConvolution, "ViewMatrix");

    glViewport(0, 0, IrradianceCubemapWidth, IrradianceCubemapWidth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 32, 32);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, *HDRCubemapEnvmap); 
    for(int i = 0; i < 6; ++i)
    {
        SendMat4(ViewLoc, ViewDirs[i]);
        CheckGLError("ViewMatrix Cubemap Convolution");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                *HDRIrradianceEnvmap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, SkyboxCube.IndexCount, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    glDeleteTextures(1, &HDRLatlongEnvmap);
    glDeleteProgram(ProgramLatlong2Cubemap);
    glDeleteProgram(ProgramCubemapConvolution);
    DestroyFramebuffer(&FBOEnvmap);
    DestroyMesh(&SkyboxCube);
}


// ADDITIONAL IMPLEMENTATION
#include "model.cpp"
