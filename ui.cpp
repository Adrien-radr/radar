#ifndef UI_CPP
#define UI_CPP

#define UI_STACK_SIZE Megabytes(8)

struct ui_render_info
{
    uint32 VertexCount;
    uint32 IndexCount;
    uint32 TextureID;
    uint32 FontTextureID;
    col4f  Color;
    bool   IsText;
};

struct ui_vertex
{
    vec3f Position;
    vec2f Texcoord;
};

game_context *uiContext;
uint32 static uiProgram;
uint32 static uiProjMatrixUniformLoc;
uint32 static uiColorUniformLoc;
uint32 static uiVAO;
uint32 static uiVBO[2];

// NOTE - This is what is stored each frame in scratch Memory
// It stacks draw commands with this layout :
// 1 ui_render_info
// 1 array of ui_vertex
// 1 array of uint16 for the indices
void *uiRenderCmd;
uint32 uiRenderCmdCount;
memory_arena uiRenderCmdArena;



void uiInit(game_context *Context)
{
    uiContext = Context;
    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(2, uiVBO);

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ui_vertex), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ui_vertex), (GLvoid*)sizeof(vec3f));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void uiReloadShaders(game_memory *Memory, path ExecFullPath)
{
    path VSPath, FSPath;
    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/ui_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/ui_frag.glsl");
    uiProgram = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(uiProgram);
    SendInt(glGetUniformLocation(uiProgram, "Texture0"), 0);

    uiProjMatrixUniformLoc = glGetUniformLocation(uiProgram, "ProjMatrix");
    uiColorUniformLoc = glGetUniformLocation(uiProgram, "Color");
}

void uiBeginFrame(game_memory *Memory, game_input *Input)
{
    uiRenderCmd = PushArenaData(&Memory->ScratchArena, UI_STACK_SIZE);    
    InitArena(&uiRenderCmdArena, UI_STACK_SIZE, uiRenderCmd);
    uiRenderCmdCount = 0;
}

void uiMakeText(char const *Text, vec3i Position, int MaxWidth)
{
    uint32 MsgLength = strlen(Text);
    uint32 VertexCount = MsgLength * 4;
    uint32 IndexCount = MsgLength * 6;

    ui_render_info *RenderInfo = (ui_render_info*)PushArenaStruct(&uiRenderCmdArena, ui_render_info);
    ui_vertex *VertData = (ui_vertex*)PushArenaData(&uiRenderCmdArena, VertexCount * sizeof(ui_vertex));
    // NOTE - Because of USHORT max is 65535, Cant fit more than 10922 characters per Text
    uint16 *IdxData = (uint16*)PushArenaData(&uiRenderCmdArena, IndexCount * sizeof(uint16));

    RenderInfo->VertexCount = VertexCount;
    RenderInfo->IndexCount = IndexCount;
    RenderInfo->TextureID = 0;
    RenderInfo->FontTextureID = uiContext->DefaultFontTexture;
    RenderInfo->Color = vec4f(0, 0, 0, 1);
    RenderInfo->IsText = true;

    vec3i DisplayPos = vec3i(Position.x, uiContext->WindowHeight - Position.y, Position.z);
    FillDisplayTextInterleaved(Text, MsgLength, &uiContext->DefaultFont, DisplayPos, MaxWidth, (real32*)VertData, IdxData);

    ++uiRenderCmdCount;
}

void uiBeginPanel(char const *PanelTitle, vec3i Position, vec2i Size, col4f Color)
{
    ui_render_info *RenderInfo = (ui_render_info*)PushArenaStruct(&uiRenderCmdArena, ui_render_info);
    ui_vertex *VertData = (ui_vertex*)PushArenaData(&uiRenderCmdArena, 4 * sizeof(ui_vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&uiRenderCmdArena, 6 * sizeof(uint16));

    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = uiContext->DefaultDiffuseTexture;
    RenderInfo->FontTextureID = 0;
    RenderInfo->Color = Color;
    RenderInfo->IsText = false;

    IdxData[0] = 0; IdxData[1] = 1; IdxData[2] = 2; IdxData[3] = 0; IdxData[4] = 2; IdxData[5] = 3; 
    int Y = uiContext->WindowHeight;
    VertData[0] = ui_vertex { vec3f(Position.x,          Y - Position.y,          Position.z), vec2f(0.f, 0.f) };
    VertData[1] = ui_vertex { vec3f(Position.x,          Y - Position.y - Size.y, Position.z), vec2f(0.f, 1.f) };
    VertData[2] = ui_vertex { vec3f(Position.x + Size.x, Y - Position.y - Size.y, Position.z), vec2f(1.f, 1.f) };
    VertData[3] = ui_vertex { vec3f(Position.x + Size.x, Y - Position.y,          Position.z), vec2f(1.f, 0.f) };

    ++uiRenderCmdCount;

    // Add panel title as text
    uiMakeText(PanelTitle, Position + vec3i(5, 5, 1), Size.x - 5);
}

void uiEndPanel()
{
    // TODO - Keep track of per-panel info, stacking, layout etc
}

void *RenderCmdOffset(uint8 *CmdList, size_t *OffsetAccum, size_t Size)
{
    void* Ptr = (void*)(CmdList + *OffsetAccum);
    *OffsetAccum += Size;
    return Ptr;
}

void uiDraw()
{
    glUseProgram(uiProgram);
    // TODO - ProjMatrix updated only when resize happen
    {
        SendMat4(uiProjMatrixUniformLoc, uiContext->ProjectionMatrix2D);
        CheckGLError("ProjMatrix UI");
    }

    glBindVertexArray(uiVAO);
    uint8 *Cmd = (uint8*)uiRenderCmd;
    for(uint32 i = 0; i < uiRenderCmdCount; ++i)
    {
        size_t Offset = 0;
        ui_render_info *RenderInfo = (ui_render_info*)RenderCmdOffset(Cmd, &Offset, sizeof(ui_render_info));
        ui_vertex *VertData = (ui_vertex*)RenderCmdOffset(Cmd, &Offset, RenderInfo->VertexCount * sizeof(ui_vertex));
        uint16 *IdxData = (uint16*)RenderCmdOffset(Cmd, &Offset, RenderInfo->IndexCount * sizeof(uint16));

        glBindBuffer(GL_ARRAY_BUFFER, uiVBO[1]);
        glBufferData(GL_ARRAY_BUFFER, RenderInfo->VertexCount * sizeof(ui_vertex), (GLvoid*)VertData, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiVBO[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, RenderInfo->IndexCount * sizeof(uint16), (GLvoid*)IdxData, GL_STREAM_DRAW);

        if(RenderInfo->IsText)
            glBindTexture(GL_TEXTURE_2D, RenderInfo->FontTextureID);
        else
            glBindTexture(GL_TEXTURE_2D, RenderInfo->TextureID);

        SendVec4(uiColorUniformLoc, RenderInfo->Color);
        glDrawElements(GL_TRIANGLES, RenderInfo->IndexCount, GL_UNSIGNED_SHORT, 0);

        Cmd += Offset;
    }
    glBindVertexArray(0);
}

#endif
