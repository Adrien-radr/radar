#include "ui.h"
#include "utils.h"


#define UI_STACK_SIZE Megabytes(8)


namespace ui {
game_context static *Context;
uint32 static Program;
uint32 static ProjMatrixUniformLoc;
uint32 static ColorUniformLoc;
uint32 static VAO;
uint32 static VBO[2];

// NOTE - This is what is stored each frame in scratch Memory
// It stacks draw commands with this layout :
// 1 render_info
// 1 array of vertex
// 1 array of uint16 for the indices
void static *RenderCmd;
uint32 static RenderCmdCount;
memory_arena static RenderCmdArena;

struct render_info
{
    uint32 VertexCount;
    uint32 IndexCount;
    uint32 TextureID;
    col4f  Color;
};

struct vertex
{
    vec3f Position;
    vec2f Texcoord;
};

vertex UIVertex(vec3f const &Position, vec2f const &Texcoord)
{
    vertex V = { Position, Texcoord };
    return V;
}

void Init(game_context *Context)
{
    ui::Context = Context;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)sizeof(vec3f));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ReloadShaders(game_memory *Memory, game_context *Context, path ExecFullPath)
{
    path VSPath, FSPath;
    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/ui_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/ui_frag.glsl");
    Program = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program);
    SendInt(glGetUniformLocation(Program, "Texture0"), 0);

    ProjMatrixUniformLoc = glGetUniformLocation(Program, "ProjMatrix");
    ColorUniformLoc = glGetUniformLocation(Program, "Color");

    Context::RegisterShader2D(Context, Program);
}

void BeginFrame(game_memory *Memory, game_input *Input)
{
    // NOTE - reinit the frame stack for the ui
    game_system *System = (game_system*)Memory->PermanentMemPool;
    System->UIStack = (ui_frame_stack*)PushArenaStruct(&Memory->ScratchArena, ui_frame_stack);

    RenderCmd = PushArenaData(&Memory->ScratchArena, UI_STACK_SIZE);    
    InitArena(&RenderCmdArena, UI_STACK_SIZE, RenderCmd);
    RenderCmdCount = 0;
}

void MakeText(char const *Text, font *Font, vec3i Position, col4f Color, int MaxWidth)
{
    uint32 MsgLength = strlen(Text);
    uint32 VertexCount = MsgLength * 4;
    uint32 IndexCount = MsgLength * 6;

    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena, render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena, VertexCount * sizeof(vertex));
    // NOTE - Because of USHORT max is 65535, Cant fit more than 10922 characters per Text
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena, IndexCount * sizeof(uint16));

    RenderInfo->VertexCount = VertexCount;
    RenderInfo->IndexCount = IndexCount;
    RenderInfo->TextureID = Font->AtlasTextureID;
    RenderInfo->Color = Color;

    vec3i DisplayPos = vec3i(Position.x, Context->WindowHeight - Position.y, Position.z);
    FillDisplayTextInterleaved(Text, MsgLength, Font, DisplayPos, MaxWidth, (real32*)VertData, IdxData);

    ++RenderCmdCount;
}

void BeginPanel(char const *PanelTitle, vec3i Position, vec2i Size, col4f Color)
{
    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena, render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena, 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena, 6 * sizeof(uint16));

    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = Context->DefaultDiffuseTexture;
    RenderInfo->Color = Color;

    IdxData[0] = 0; IdxData[1] = 1; IdxData[2] = 2; IdxData[3] = 0; IdxData[4] = 2; IdxData[5] = 3; 
    int Y = Context->WindowHeight;
    VertData[0] = UIVertex(vec3f(Position.x,          Y - Position.y,          Position.z), vec2f(0.f, 0.f));
    VertData[1] = UIVertex(vec3f(Position.x,          Y - Position.y - Size.y, Position.z), vec2f(0.f, 1.f));
    VertData[2] = UIVertex(vec3f(Position.x + Size.x, Y - Position.y - Size.y, Position.z), vec2f(1.f, 1.f));
    VertData[3] = UIVertex(vec3f(Position.x + Size.x, Y - Position.y,          Position.z), vec2f(1.f, 0.f));

    ++RenderCmdCount;

    // Add panel title as text
    MakeText(PanelTitle, &Context->DefaultFont, Position + vec3i(5, 5, 1), col4f(0, 0, 0, 1), Size.x - 5);
}

void EndPanel()
{
    // TODO - Keep track of per-panel info, stacking, layout etc
}

void *RenderCmdOffset(uint8 *CmdList, size_t *OffsetAccum, size_t Size)
{
    void* Ptr = (void*)(CmdList + *OffsetAccum);
    *OffsetAccum += Size;
    return Ptr;
}

void Draw()
{
    glUseProgram(Program);

    glBindVertexArray(VAO);
    uint8 *Cmd = (uint8*)RenderCmd;
    for(uint32 i = 0; i < RenderCmdCount; ++i)
    {
        size_t Offset = 0;
        render_info *RenderInfo = (render_info*)RenderCmdOffset(Cmd, &Offset, sizeof(render_info));
        vertex *VertData = (vertex*)RenderCmdOffset(Cmd, &Offset, RenderInfo->VertexCount * sizeof(vertex));
        uint16 *IdxData = (uint16*)RenderCmdOffset(Cmd, &Offset, RenderInfo->IndexCount * sizeof(uint16));

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, RenderInfo->VertexCount * sizeof(vertex), (GLvoid*)VertData, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, RenderInfo->IndexCount * sizeof(uint16), (GLvoid*)IdxData, GL_STREAM_DRAW);

        glBindTexture(GL_TEXTURE_2D, RenderInfo->TextureID);

        SendVec4(ColorUniformLoc, RenderInfo->Color);
        glDrawElements(GL_TRIANGLES, RenderInfo->IndexCount, GL_UNSIGNED_SHORT, 0);

        Cmd += Offset;
    }
    glBindVertexArray(0);
}
}
