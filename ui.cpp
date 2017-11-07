#include "ui.h"
#include "utils.h"

#define UI_STACK_SIZE Megabytes(8)
#define UI_MAX_PANELS 32
#define UI_PARENT_SIZE 10

#include "ui_theme.cpp"

namespace ui {
game_context static *Context;
game_input static   *Input;

struct input_state
{
    void   *ID;
    uint16 Idx;
    int16  Priority;
};

uint16 PanelCount;         // Total number of panels ever registered
void *ParentID[UI_PARENT_SIZE]; // ID stack of the parent of the current widgets, changes when a panel is begin and ended
int16 PanelOrder[UI_MAX_PANELS];
int16 RenderOrder[UI_MAX_PANELS];
uint16 ParentLayer;             // Current Parent layer widgets are attached to
input_state Hover;
input_state HoverNext;
input_state Focus;
input_state FocusNext;
int16 ForcePanelFocus;

bool MouseHold;

int16 LastRootWidget; // Address of the last widget not attached to anything

// TMP
char HoverTitleCurrent[64];
char HoverTitleNext[64];
char FocusTitleCurrent[64];
char FocusTitleNext[64];

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
void static *RenderCmd[UI_MAX_PANELS];
uint32 static RenderCmdCount[UI_MAX_PANELS];
memory_arena static RenderCmdArena[UI_MAX_PANELS];

enum widget_type {
    WIDGET_PANEL,
    WIDGET_TEXT,
    WIDGET_BUTTON,
    WIDGET_TITLEBAR,
    WIDGET_BORDER,
    WIDGET_SLIDER,
    WIDGET_COUNT
};

struct render_info
{
    uint32      VertexCount;
    uint32      IndexCount;
    uint32      TextureID;
    col4f       Color;
    void        *ID;
    void        *ParentID;
    widget_type Type;
    vec2i       Position;
    vec2i       Size;
    int32       Flags;
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

void Init(game_memory *Memory, game_context *Context)
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

    Hover = { NULL, 0, 0 };
    HoverNext = { NULL, 0, 0 };
    Focus = { NULL, 0, 0 };
    FocusNext = { NULL, 0, 0 };
    memset(HoverTitleNext, 0, 64);
    strncpy(FocusTitleNext, "None", 64);
    memset(FocusTitleCurrent, 0, 64);

    // 1 panel at the start : the 'backpanel' where floating widgets are stored
    PanelCount = 1;

    ParentLayer = 0;
    memset(ParentID, 0, sizeof(void*) * UI_PARENT_SIZE);

    for(int16 i = 0; i < UI_MAX_PANELS; ++i)
    {
        PanelOrder[i] = i;
    }

    MouseHold = false;

    path ConfigPath;
    MakeRelativePath(&Memory->ResourceHelper, ConfigPath, Memory->Config.UIConfigFilename);
    ParseUIConfig(Memory, Context, ConfigPath);
}

void ReloadShaders(game_memory *Memory, game_context *Context)
{
    path VSPath, FSPath;
    MakeRelativePath(&Memory->ResourceHelper, VSPath, "data/shaders/ui_vert.glsl");
    MakeRelativePath(&Memory->ResourceHelper, FSPath, "data/shaders/ui_frag.glsl");
    Program = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(Program);
    SendInt(glGetUniformLocation(Program, "Texture0"), 0);

    ProjMatrixUniformLoc = glGetUniformLocation(Program, "ProjMatrix");
    ColorUniformLoc = glGetUniformLocation(Program, "Color");

    context::RegisterShader2D(Context, Program);
}

void BeginFrame(game_memory *Memory, game_input *Input)
{
    // NOTE - reinit the frame stack for the ui
    game_system *System = (game_system*)Memory->PermanentMemPool;
    System->UIStack = (frame_stack*)PushArenaStruct(&Memory->ScratchArena, frame_stack);

    // TODO -  probably this can be done with 1 'alloc' and redirections in the buffer
    for(uint32 p = 0; p < UI_MAX_PANELS; ++p)
    {
        RenderCmd[p] = PushArenaData(&Memory->ScratchArena, UI_STACK_SIZE/UI_MAX_PANELS);
        InitArena(&RenderCmdArena[p], UI_STACK_SIZE/UI_MAX_PANELS, RenderCmd[p]);
    }
    memset(RenderCmdCount, 0, UI_MAX_PANELS * sizeof(uint32));

    ui::Input = Input;
    context::SetCursor(Context, context::CURSOR_NORMAL);

    LastRootWidget = 0;
    ForcePanelFocus = 0;
    ParentLayer = 0;
    Hover = HoverNext;
    HoverNext = { NULL, 0, 0 };
    Focus = FocusNext;

    // TMP
    strncpy(HoverTitleCurrent, HoverTitleNext, 64);
    strncpy(HoverTitleNext, "None", 64);
    strncpy(FocusTitleCurrent, FocusTitleNext, 64);

    char Text[64];
    snprintf(Text, 64, "Current Hover : %s", HoverTitleCurrent);
    MakeText(NULL, Text, FONT_DEFAULT, vec3f(600, 10, 0.0001f), COLOR_DEBUGFG, Context->WindowWidth);

    snprintf(Text, 64, "Current Focus : %s", FocusTitleCurrent);
    MakeText(NULL, Text, FONT_DEFAULT, vec3f(600, 24, 0.0001f), COLOR_DEBUGFG, Context->WindowWidth);

    // Reset the FocusID to None if we have a mouse click, future frame panels will change that
    if(MOUSE_HIT(Input->MouseLeft))
    {
        FocusNext = { NULL, 0, PanelOrder[0] };
        strncpy(FocusTitleNext, "None", 64);
    }
    if(MOUSE_UP(Input->MouseLeft))
    {
        MouseHold = false;
    }
}

static bool IsRootWidget()
{
    return ParentLayer == 0;
}

render_info *GetParentRenderInfo(int16 ParentIdx)
{
    return (render_info*)(RenderCmdArena[ParentIdx].BasePtr);
}

static bool PointInRectangle(const vec2f &Point, const vec2f &TopLeft, const vec2f &BottomRight)
{
    if(Point.x >= TopLeft.x && Point.x < BottomRight.x)
        if(Point.y > BottomRight.y && Point.y <= TopLeft.y)
            return true;
    return false;
}

void FillSquare(vertex *VertData, uint16 *IdxData, int V1, int I1, vec2f const &TL, vec2f const &BR, float TexScale = 1.f)
{
    IdxData[I1+0] = V1+0; IdxData[I1+1] = V1+1; IdxData[I1+2] = V1+2;
    IdxData[I1+3] = V1+0; IdxData[I1+4] = V1+2; IdxData[I1+5] = V1+3;

    VertData[V1+0] = UIVertex(vec3f(TL.x, TL.y, 0), vec2f(0.f, 0.f));
    VertData[V1+1] = UIVertex(vec3f(TL.x, BR.y, 0), vec2f(0.f, TexScale));
    VertData[V1+2] = UIVertex(vec3f(BR.x, BR.y, 0), vec2f(TexScale, TexScale));
    VertData[V1+3] = UIVertex(vec3f(BR.x, TL.y, 0), vec2f(TexScale, 0.f));
}

void MakeText(void *ID, char const *Text, theme_font Font, vec3i Position, theme_color Color, int MaxWidth)
{
    MakeText(ID, Text, Font, Position, GetColor(Color), MaxWidth);
}

void MakeText(void *ID, char const *Text, theme_font FontStyle, vec3i Position, col4f Color, int MaxWidth)
{
    uint32 const MsgLength = strlen(Text);
    uint32 const VertexCount = MsgLength * 4;
    uint32 const IndexCount = MsgLength * 6;

    bool const NoParent = IsRootWidget();
    uint16 const PanelIdx = LastRootWidget;


    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[PanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[PanelIdx], VertexCount * sizeof(vertex));
    // NOTE - Because of USHORT max is 65535, Cant fit more than 10922 characters per Text
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[PanelIdx], IndexCount * sizeof(uint16));

    font *Font = GetFont(FontStyle);

    RenderInfo->Type = WIDGET_TEXT;
    RenderInfo->VertexCount = VertexCount;
    RenderInfo->IndexCount = IndexCount;
    RenderInfo->TextureID = Font->AtlasTextureID;
    RenderInfo->Color = Color;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = NoParent ? NULL : ParentID[ParentLayer];

    vec3i DisplayPos = vec3i(Position.x, Context->WindowHeight - Position.y, Position.z);
    FillDisplayTextInterleaved(Text, MsgLength, Font, DisplayPos, MaxWidth, (real32*)VertData, IdxData);

    ++(RenderCmdCount[PanelIdx]);
}

void MakeTitlebar(void *ID, char const *PanelTitle, vec3i Position, vec2i Size, col4f Color)
{
    int16 const ParentPanelIdx = LastRootWidget;
    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[ParentPanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_TITLEBAR;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Color;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = ParentID[ParentLayer];

    int const Y = Context->WindowHeight;
    vec2f TL(Position.x, Y - Position.y);
    vec2f BR(Position.x + Size.x, Y - Position.y - Size.y);

    FillSquare(VertData, IdxData, 0, 0, TL, BR);
    ++(RenderCmdCount[ParentPanelIdx]);

    // Add panel title as text
    MakeText(NULL, PanelTitle, FONT_DEFAULT, Position + vec3f(4,4,0), Theme.PanelFG, Size.x - 4);
}

void MakeBorder(vec2f const &OrigTL, vec2f const &OrigBR)
{
    int16 const ParentPanelIdx = LastRootWidget;
    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[ParentPanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 16 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 24 * sizeof(uint16));

    RenderInfo->Type = WIDGET_BORDER;
    RenderInfo->VertexCount = 16;
    RenderInfo->IndexCount = 24;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Theme.BorderBG;
    RenderInfo->ID = NULL;
    RenderInfo->ParentID = ParentID[ParentLayer];

    vec2f TL(OrigTL);
    vec2f BR(OrigBR.x, OrigTL.y-1);
    FillSquare(VertData, IdxData, 0, 0, TL, BR);

    TL = vec2f(OrigTL.x, OrigBR.y+1);
    BR = vec2f(OrigBR);
    FillSquare(VertData, IdxData, 4, 6, TL, BR);

    TL = vec2f(OrigTL.x, OrigTL.y-1);
    BR = vec2f(OrigTL.x+1, OrigBR.y+1);
    FillSquare(VertData, IdxData, 8, 12, TL, BR);

    TL = vec2f(OrigBR.x-1, OrigTL.y-1);
    BR = vec2f(OrigBR.x, OrigBR.y+1);
    FillSquare(VertData, IdxData, 12, 18, TL, BR);

    ++(RenderCmdCount[ParentPanelIdx]);
}

void MakeSlider(real32 *ID, real32 MinVal, real32 MaxVal)
{
    int16 const ParentPanelIdx = LastRootWidget;
    if(ParentPanelIdx == 0) return;

    // TODO - this is pretty redundant, all this work for 2 squares...
    // Maybe allow color attribute to vertices instead of having it uniform

    // NOTE - Background square
    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[ParentPanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_SLIDER;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Theme.SliderBG;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = ParentID[ParentLayer];

    render_info *ParentRI = GetParentRenderInfo(ParentPanelIdx);
    int const Y = Context->WindowHeight;
    int const TitlebarOffset = ParentRI->Flags & DECORATION_TITLEBAR ? 20 : 0;
    vec2i Size = vec2i(5, ParentRI->Size.y - TitlebarOffset - 2);
    vec2i Pos(ParentRI->Position.x + ParentRI->Size.x - Size.x - 1,
              Y - ParentRI->Position.y - TitlebarOffset - 1);

    vec2f TL(Pos);
    vec2f BR(Pos.x+Size.x, Pos.y-Size.y);

    FillSquare(VertData, IdxData, 0, 0, TL, BR);
    ++(RenderCmdCount[ParentPanelIdx]);

    // NOTE - Foreground slider
    RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[ParentPanelIdx], render_info);
    VertData = (vertex*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 4 * sizeof(vertex));
    IdxData = (uint16*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_SLIDER;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Theme.SliderFG;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = ParentID[ParentLayer];

    real32 const HalfSliderHeight = 10.f;
    //real32 const X = Min(MaxVal Max(MinVal *ID));
    real32 const Ratio = 1.f - ((*ID - MinVal) / (MaxVal - MinVal));
    real32 const PXHeight = Size.y - 2.f * HalfSliderHeight;
    TL = vec2f(Pos.x, Pos.y - HalfSliderHeight - Ratio * PXHeight + HalfSliderHeight);
    BR = vec2f(Pos.x + Size.x, Pos.y - HalfSliderHeight - Ratio * PXHeight - HalfSliderHeight);

    FillSquare(VertData, IdxData, 0, 0, TL, BR);
    ++(RenderCmdCount[ParentPanelIdx]);

    //vec2i TL(ParentRI->Position.x, Y - ParentRI->Position.y);
    //vec2i BR(ParentRI->Position.x + ParentRI->Size.x, Y - ParentRI->Position.y - ParentRI->Size.y);
    if(Hover.ID == ParentRI->ID)
    {
        //vec2f MousePos(Input->MousePosX, Y - Input->MousePosY);
        if(Input->MouseDZ!=0)// && PointInRectangle(MousePos, TL, BR))
        {
            *ID += 1.0f * Input->MouseDZ;
            *ID = Min(MaxVal, Max(MinVal, *ID));
        }
    }
}

bool MakeButton(uint32 *ID, char *ButtonText, vec2i PositionOffset, vec2i Size)
{
    int16 const ParentPanelIdx = LastRootWidget;
    if(ParentPanelIdx == 0) return false;

    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[ParentPanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_BUTTON;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = *ID > 0 ? Theme.ButtonPressedBG : Theme.ButtonBG;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = ParentID[ParentLayer];

    render_info *ParentRI = GetParentRenderInfo(ParentPanelIdx);
    int const Y = Context->WindowHeight;
    int const TitlebarOffset = ParentRI->Flags & DECORATION_TITLEBAR ? 20 : 0;
    vec2f const OffsetPos(ParentRI->Position.x + PositionOffset.x + 1, ParentRI->Position.y + PositionOffset.y + TitlebarOffset + 1);
    vec2f const TL(OffsetPos.x, Y - OffsetPos.y);
    vec2f const BR(TL.x + Size.x, TL.y - Size.y);

    FillSquare(VertData, IdxData, 0, 0, TL + vec2i(1,-1), BR + vec2i(-1,1));
    ++(RenderCmdCount[ParentPanelIdx]);

    MakeBorder(TL, BR);
    MakeText(NULL, ButtonText, FONT_DEFAULT, vec3i(OffsetPos.x + 4, OffsetPos.y + 4, 0), Theme.PanelFG, Size.x - 4);

    vec2f const MousePos(Input->MousePosX, Y - Input->MousePosY);
    if(Hover.ID == ParentRI->ID)
    {
        if(MOUSE_HIT(Input->MouseLeft) && PointInRectangle(MousePos, TL, BR))
        {
            *ID = 1;
        }
        else if(MOUSE_UP(Input->MouseLeft) && PointInRectangle(MousePos, TL, BR))
        {
            *ID = 0;
            return true;
        }
    }

    // Release button (without activation) if it was pressed but not released within bounds
    if(*ID > 0 && MOUSE_UP(Input->MouseLeft) && !PointInRectangle(MousePos, TL, BR))
    {
        *ID = 0;
    }

    return false;
}

void MakeImage(real32 *ID, uint32 TextureID)
{
    int16 const ParentPanelIdx = LastRootWidget;
    if(ParentPanelIdx == 0) return;

    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[ParentPanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[ParentPanelIdx], 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_BUTTON;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = TextureID;
    RenderInfo->Color = Theme.White;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = ParentID[ParentLayer];

    render_info *ParentRI = GetParentRenderInfo(ParentPanelIdx);
    int const Y = Context->WindowHeight;
    int const TitlebarOffset = ParentRI->Flags & DECORATION_TITLEBAR ? 20 : 0;
    vec2f TL(ParentRI->Position.x+1, Y - ParentRI->Position.y - TitlebarOffset - 1);
    vec2f BR(ParentRI->Position.x + ParentRI->Size.x - 1, Y - ParentRI->Position.y - ParentRI->Size.y);

    FillSquare(VertData, IdxData, 0, 0, TL, BR, 1.f/ *ID);
    ++(RenderCmdCount[ParentPanelIdx]);

    if(Hover.ID == ParentRI->ID)
    {
        if(Input->MouseDZ != 0)
        {
            *ID *= (1.f + 0.1f * Input->MouseDZ);
            *ID = Max(0.0001f, *ID);
        }
    }
}

void BeginPanel(uint32 *ID, char const *PanelTitle, vec3i *Position, vec2i Size, decoration_flag Flags)
{
    Assert(PanelCount < UI_MAX_PANELS);
    uint32 PanelIdx;

    // NOTE - Init stage of the panel, storing the first time Panel Idx, as well as forcing Focus on it
    if(*ID == 0)
    {
        PanelIdx = PanelCount++;
        *ID = PanelIdx;
        ForcePanelFocus = PanelIdx;
    }
    else
    {
        PanelIdx = *ID;
    }

    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[PanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[PanelIdx], 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[PanelIdx], 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_PANEL;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Theme.PanelBG;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = NULL;
    RenderInfo->Position = vec2i(Position->x, Position->y);
    RenderInfo->Size = Size;
    RenderInfo->Flags = Flags;
    LastRootWidget = *ID;

    int const Y = Context->WindowHeight;
    vec2f TL(Position->x, Y - Position->y);
    vec2f BR(Position->x + Size.x, Y - Position->y - Size.y);

    FillSquare(VertData, IdxData, 0, 0, TL, BR);
    ++(RenderCmdCount[PanelIdx]);
    ParentID[ParentLayer++] = ID;

    MakeBorder(TL, BR);

    if(Flags & DECORATION_TITLEBAR)
    {
        MakeTitlebar(NULL, PanelTitle, *Position + vec3f(1, 1, 0), vec2i(Size.x - 2, 20), Theme.TitlebarBG);
    }

    if(HoverNext.Priority <= PanelOrder[PanelIdx])
    {
        vec2f MousePos(Input->MousePosX, Y - Input->MousePosY);
        if(PointInRectangle(MousePos, TL, BR))
        {
            HoverNext.ID = ID;
            HoverNext.Priority = PanelOrder[PanelIdx];
            HoverNext.Idx = PanelIdx;
            strncpy(HoverTitleNext, PanelTitle, 64);

            // Test for Titlebar click
            vec2f TB_TL = TL + vec2f(1,1);
            vec2f TB_BR(Position->x + Size.x - 2, Y - Position->y - 20);
            if(Flags & DECORATION_TITLEBAR && ID == Hover.ID && PointInRectangle(MousePos, TB_TL, TB_BR))
            {
                if(MOUSE_HIT(Input->MouseLeft))
                {
                    MouseHold = true;
                }
            }
        }
    }

    if(Focus.ID == ID && MouseHold)
    {
        Position->x += Input->MouseDX;
        Position->y += Input->MouseDY;
    }
}

void EndPanel()
{
    --ParentLayer;
}

// Reorders the panel focus and render orders by pushing panel Idx to the front
static void FocusReorder(int16 Idx)
{
    int16 OldPriority = PanelOrder[Idx];

    for(uint32 p = 1; p < PanelCount; ++p)
        if(PanelOrder[p] > OldPriority)
            --PanelOrder[p];
    PanelOrder[Idx] = PanelCount - 1;

    for(uint32 i = 0; i < PanelCount; ++i)
    {
        RenderOrder[PanelOrder[i]] = i;
    }
}

static void Update()
{
    if(ForcePanelFocus > 0)
    { // NOTE - push focus on a specific panel if needed (usually when it first appear or if the caller asks it)
        FocusReorder(ForcePanelFocus);
    }

    if(MOUSE_HIT(Input->MouseLeft))
    {
        FocusNext = Hover;
        strncpy(FocusTitleNext, HoverTitleCurrent, 64);

        // Reorder panel rendering order if focus has changed
        if(Hover.Priority > 0 && Hover.Priority < (PanelCount-1))
        {
            FocusReorder(Hover.Idx);
        }
    }
}

static void *RenderCmdOffset(uint8 *CmdList, size_t *OffsetAccum, size_t Size)
{
    void* Ptr = (void*)(CmdList + *OffsetAccum);
    *OffsetAccum += Size;
    return Ptr;
}

void Draw()
{
    Update();
    glUseProgram(Program);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(VAO);
    // TODO - 1 DrawCall for the whole AI ? just need to prepare the buffer with each RenderCmd beforehand
    for(int p = 0; p < PanelCount; ++p)
    {
        int16 OrdPanelIdx = RenderOrder[p];
        uint8 *Cmd = (uint8*)RenderCmd[OrdPanelIdx];
        for(uint32 i = 0; i < RenderCmdCount[OrdPanelIdx]; ++i)
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
    }
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}
}
