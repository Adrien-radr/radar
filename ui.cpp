#include "ui.h"
#include "utils.h"


#define UI_STACK_SIZE Megabytes(8)
#define UI_PARENT_SIZE 10
#define UI_MAX_PANELS 100


#define UI_Z_DOWN 0.001
#define UI_Z_UP 0.1

namespace ui {
game_context static *Context;
game_input static   *Input;

uint32 PanelCount;
uint32 PanelCountNext;
void *FramePanels[UI_MAX_PANELS]; // Panels asked to be drawn from the code
void *PanelOrder[UI_MAX_PANELS];  // Reordering of the above according to focus
void *ParentID[UI_PARENT_SIZE]; // ID stack of the parent of the current widgets, changes when a panel is begin and ended
uint32 ParentLayer;             // Current Parent layer widgets are attached to
void *HoverID;      // ID of the widget that has the hover focus (mouse hovered)
void *HoverNextID;
void *FocusID;      // ID of the widget that has the focus (clicked)
void *FocusNextID;      

void *LastRootWidget; // Address of the last widget not attached to anything

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
void static *RenderCmd;
void static *OrderedRenderCmd;
uint32 static RenderCmdCount;
memory_arena static RenderCmdArena;

enum widget_type {
    WIDGET_PANEL,
    WIDGET_TEXT,
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

    HoverID = HoverNextID = NULL;
    FocusID = FocusNextID = NULL;
    memset(HoverTitleNext, 0, 64);
    strncpy(FocusTitleNext, "None", 64);
    memset(FocusTitleCurrent, 0, 64);

    PanelCount = PanelCountNext = 0;
    memset(PanelOrder, NULL, sizeof(void*) * UI_MAX_PANELS);

    ParentLayer = 0;
    memset(ParentID, NULL, sizeof(void*) * UI_PARENT_SIZE);
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
    LastRootWidget = RenderCmd;

    ui::Input = Input;

    ParentLayer = 0;
    HoverID = HoverNextID;
    HoverNextID = NULL;
    FocusID = FocusNextID;

    PanelCount = PanelCountNext
    PanelCountNext = 0;
    memset(FramePanels, NULL, sizeof(void*) * UI_MAX_PANELS);

    // TMP
    strncpy(HoverTitleCurrent, HoverTitleNext, 64);
    strncpy(HoverTitleNext, "None", 64);
    strncpy(FocusTitleCurrent, FocusTitleNext, 64);

    char Text[64];
    snprintf(Text, 64, "Current Hover : %s", HoverTitleCurrent);
    MakeText(Text, Context->RenderResources.DefaultFont, vec3f(600, 10, 0), Context->GameConfig->DebugFontColor, Context->WindowWidth);

    snprintf(Text, 64, "Current Focus : %s", FocusTitleCurrent);
    MakeText(Text, Context->RenderResources.DefaultFont, vec3f(600, 24, 0), Context->GameConfig->DebugFontColor, Context->WindowWidth);

    // Reset the FocusID to None if we have a mouse click, future frame widgets will change that
    if(MOUSE_DOWN(Input->MouseLeft))
    {
        FocusNextID = NULL;
        strncpy(FocusTitleNext, "None", 64);
    }
}

static bool IsRootWidget()
{
    return ParentLayer == 0;
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

    RenderInfo->Type = WIDGET_TEXT;
    RenderInfo->VertexCount = VertexCount;
    RenderInfo->IndexCount = IndexCount;
    RenderInfo->TextureID = Font->AtlasTextureID;
    RenderInfo->Color = Color;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = IsRootWidget() ? NULL : ParentID[ParentLayer];

    vec3i DisplayPos = vec3i(Position.x, Context->WindowHeight - Position.y, Position.z);
    FillDisplayTextInterleaved(Text, MsgLength, Font, DisplayPos, MaxWidth, (real32*)VertData, IdxData);

    ++RenderCmdCount;
}

static bool PointInRectangle(const vec2f &Point, const vec2f &TopLeft, const vec2f &BottomRight)
{
    if(Point.x >= TopLeft.x && Point.x < BottomRight.x)
        if(Point.y > BottomRight.y && Point.y <= TopLeft.y)
            return true;
    return false;
}

void BeginPanel(void *ID, char const *PanelTitle, vec3i Position, vec2i Size, col4f Color)
{
    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena, render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena, 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena, 6 * sizeof(uint16));

    RenderInfo->Type = WIDGET_PANEL
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Color;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = IsRootWidget() ? NULL : ParentID[ParentLayer];

    int const Y = Context->WindowHeight;
    vec2f TL(Position.x, Y - Position.y);
    vec2f BR(Position.x + Size.x, Y - Position.y - Size.y);

    IdxData[0] = 0; IdxData[1] = 1; IdxData[2] = 2; IdxData[3] = 0; IdxData[4] = 2; IdxData[5] = 3; 
    VertData[0] = UIVertex(vec3f(TL.x, TL.y, Position.z), vec2f(0.f, 0.f));
    VertData[1] = UIVertex(vec3f(TL.x, BR.y, Position.z), vec2f(0.f, 1.f));
    VertData[2] = UIVertex(vec3f(BR.x, BR.y, Position.z), vec2f(1.f, 1.f));
    VertData[3] = UIVertex(vec3f(BR.x, TL.y, Position.z), vec2f(1.f, 0.f));

    ++RenderCmdCount;
    ParentID[ParentLayer++] = ID;
    FramePanels[PanelCount++] = ID;

    // Add panel title as text
    MakeText(PanelTitle, Context->RenderResources.DefaultFont, Position + vec3i(5, 5, 1), col4f(0, 0, 0, 1), Size.x - 5);

    // TODO - push that to a 2nd pass that sorts shit
#if 0
    if(PointInRectangle(vec2f(Input->MousePosX, Y - Input->MousePosY), TL, BR))
    {
        HoverNextID = ID;
        strncpy(HoverTitleNext, PanelTitle, 64);

        if(MOUSE_DOWN(Input->MouseLeft))
        {
            FocusNextID = ID;
            strncpy(FocusTitleNext, PanelTitle, 64);
        }
    }
#endif
}

void EndPanel()
{
    // TODO - Keep track of per-panel info, stacking, layout etc
    --ParentLayer;
}

static bool FindPanelID(void *ID)
{
    for(uint32 i = 0; i < PanelCount; ++i)
    {
        if(PanelOrder[i] == ID)
            return true;
    }
    return false;
}

static void UpdateOrder()
{
    // TODO - panel removal
    if(PanelCountNext < PanelCount)
    {

    }

    // Look if all the panels asked this frame are already in the sort order
    for(uint32 i = 0; i < PanelCountNext; ++i)
    {
        if(!FindPanelID(FramePanels[i]))
        {
            // add new panels at the top of the priority order (new windows)
            PanelOrder[PanelCount++] = FramePanels[i];
        }
    }

    // Do Input handling in order
    for(uint32 i = 0; i < PanelCount; ++i)
    {
    }

    // Extend the RenderCmd Stack by same amount to prepare duplicacy
    if(RenderCmdArena.Size < RenderCmdArena.Capacity/2)
    {
        OrderedRenderCmd = PushArenaData(&RenderCmdArena, RenderCmdArena.Size);
        for(uint32 i = 0; i < RenderCmdCount; ++i)
        {
        }
    }
    else
    {
        printf("Error : UI Data Stack(%d) not large enough for UI rendering !\n", UI_STACK_SIZE);
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
    UpdateOrder();
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
