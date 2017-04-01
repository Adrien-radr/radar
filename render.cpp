#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_truetype.h"
#include "ext/stb_image.h"

struct image
{
    uint8 *Buffer;
    int32 Width;
    int32 Height;
    int32 Channels;
};

struct font
{
    uint8 *Buffer;
};

// TODO - Load Unicode characters
font LoadFont(char *Filename, real32 PixelHeight)
{
    font Font = {};

    void *Contents = ReadFileContents(Filename);
    if(Contents)
    {
        stbtt_fontinfo STBFont;
        stbtt_InitFont(&STBFont, (uint8*)Contents, 0);

        real32 PixelScale = stbtt_ScaleForPixelHeight(&STBFont, PixelHeight);

        int Width, Height, XOffset, YOffset;
        Font.Buffer = stbtt_GetCodepointBitmap(&STBFont, 0, stbtt_ScaleForPixelHeight(&STBFont, PixelHeight), 
                'A', &Width, &Height, &XOffset, &YOffset);

#if 0
        // Load all ASCII characters
        uint8 const AsciiCharCount = 127 - 33;
        for(uint8 c = 33; c < 127; ++c)
        {
            
        }
#endif
        FreeFileContents(Contents);
    }


    return Font;
}

void DestroyFont(font *Font)
{
    if(Font && Font->Buffer)
    {
        stbtt_FreeBitmap(Font->Buffer, 0);
        // TODO - Glyph bitmap packing and 1 free for all characters
        //free(Font->Buffer);
    }
}

image LoadImage(char *Filename, int32 ForceNumChannel = 0)
{
    image Image = {};
    Image.Buffer = stbi_load(Filename, &Image.Width, &Image.Height, &Image.Channels, ForceNumChannel);

    return Image;
}

void DestroyImage(image *Image)
{
    stbi_image_free(Image->Buffer);
    Image->Width = Image->Height = Image->Channels = 0;
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

