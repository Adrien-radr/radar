#include "ui.h"
#include "utils.h"


#define UI_STACK_SIZE Megabytes(8)
#define UI_MAX_PANELS 64
#define UI_PARENT_SIZE 10


#define UI_Z_DOWN 0.001
#define UI_Z_UP 0.1

namespace ui {
game_context static *Context;
game_input static   *Input;

struct input_state
{
    void   *ID;
    uint32 Priority;
};

uint32 PanelCount;
uint32 PanelCountNext;
void *ParentID[UI_PARENT_SIZE]; // ID stack of the parent of the current widgets, changes when a panel is begin and ended
uint32 PanelOrder[UI_MAX_PANELS];
uint32 RenderOrder[UI_MAX_PANELS];
uint32 ParentLayer;             // Current Parent layer widgets are attached to
input_state Hover;
input_state HoverNext;
input_state Focus;
input_state FocusNext;

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
void static *RenderCmd[UI_MAX_PANELS];
uint32 static RenderCmdCount[UI_MAX_PANELS];
memory_arena static RenderCmdArena[UI_MAX_PANELS];

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

    Hover = { NULL, 0 };
    HoverNext = { NULL, 0 };
    Focus = { NULL, 0 };
    FocusNext = { NULL, 0 };
    memset(HoverTitleNext, 0, 64);
    strncpy(FocusTitleNext, "None", 64);
    memset(FocusTitleCurrent, 0, 64);

    // 1 panel at the start : the 'backpanel' where floating widgets are stored
    PanelCount = PanelCountNext = 1;

    ParentLayer = 0;
    memset(ParentID, 0, sizeof(void*) * UI_PARENT_SIZE);

    for(uint32 i = 0; i < UI_MAX_PANELS; ++i)
    {
        PanelOrder[i] = i;
    }
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

    // TODO -  probably this can be done with 1 'alloc' and redirections in the buffer
    for(uint32 p = 0; p < UI_MAX_PANELS; ++p)
    {
        RenderCmd[p] = PushArenaData(&Memory->ScratchArena, UI_STACK_SIZE/UI_MAX_PANELS);    
        InitArena(&RenderCmdArena[p], UI_STACK_SIZE/UI_MAX_PANELS, RenderCmd[p]);
    }
    memset(RenderCmdCount, 0, UI_MAX_PANELS * sizeof(uint32));
    LastRootWidget = RenderCmd[0];

    ui::Input = Input;

    ParentLayer = 0;
    Hover = HoverNext;
    HoverNext = { NULL, 0 };
    Focus = FocusNext;

    PanelCount = PanelCountNext;
    PanelCountNext = 1;

    // TMP
    strncpy(HoverTitleCurrent, HoverTitleNext, 64);
    strncpy(HoverTitleNext, "None", 64);
    strncpy(FocusTitleCurrent, FocusTitleNext, 64);

    char Text[64];
    snprintf(Text, 64, "Current Hover : %s", HoverTitleCurrent);
    MakeText(NULL, Text, Context->RenderResources.DefaultFont, vec3f(600, 10, 0), Context->GameConfig->DebugFontColor, Context->WindowWidth);

    snprintf(Text, 64, "Current Focus : %s", FocusTitleCurrent);
    MakeText(NULL, Text, Context->RenderResources.DefaultFont, vec3f(600, 24, 0), Context->GameConfig->DebugFontColor, Context->WindowWidth);

    // Reset the FocusID to None if we have a mouse click, future frame widgets will change that
    if(MOUSE_DOWN(Input->MouseLeft))
    {
        FocusNext = { NULL, PanelOrder[0] };
        strncpy(FocusTitleNext, "None", 64);
    }
}

static bool IsRootWidget()
{
    return ParentLayer == 0;
}

void MakeText(void *ID, char const *Text, font *Font, vec3i Position, col4f Color, int MaxWidth)
{
    uint32 const MsgLength = strlen(Text);
    uint32 const VertexCount = MsgLength * 4;
    uint32 const IndexCount = MsgLength * 6;

    bool const NoParent = IsRootWidget();
    uint32 const PanelIdx = NoParent ? 0 : PanelCount;


    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[PanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[PanelIdx], VertexCount * sizeof(vertex));
    // NOTE - Because of USHORT max is 65535, Cant fit more than 10922 characters per Text
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[PanelIdx], IndexCount * sizeof(uint16));

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

static bool PointInRectangle(const vec2f &Point, const vec2f &TopLeft, const vec2f &BottomRight)
{
    if(Point.x >= TopLeft.x && Point.x < BottomRight.x)
        if(Point.y > BottomRight.y && Point.y <= TopLeft.y)
            return true;
    return false;
}

void BeginPanel(void *ID, char const *PanelTitle, vec3i Position, vec2i Size, col4f Color)
{
    Assert(PanelCount < UI_MAX_PANELS);
    uint32 const PanelIdx = PanelCount;

    render_info *RenderInfo = (render_info*)PushArenaStruct(&RenderCmdArena[PanelIdx], render_info);
    vertex *VertData = (vertex*)PushArenaData(&RenderCmdArena[PanelIdx], 4 * sizeof(vertex));
    uint16 *IdxData = (uint16*)PushArenaData(&RenderCmdArena[PanelIdx], 6 * sizeof(uint16));

    bool NoParent = IsRootWidget();

    RenderInfo->Type = WIDGET_PANEL;
    RenderInfo->VertexCount = 4;
    RenderInfo->IndexCount = 6;
    RenderInfo->TextureID = *Context->RenderResources.DefaultDiffuseTexture;
    RenderInfo->Color = Color;
    RenderInfo->ID = ID;
    RenderInfo->ParentID = NoParent ? NULL : ParentID[ParentLayer];

    if(NoParent)
    {
        LastRootWidget = ID;
    }

    int const Y = Context->WindowHeight;
    vec2f TL(Position.x, Y - Position.y);
    vec2f BR(Position.x + Size.x, Y - Position.y - Size.y);

    IdxData[0] = 0; IdxData[1] = 1; IdxData[2] = 2; IdxData[3] = 0; IdxData[4] = 2; IdxData[5] = 3; 
    VertData[0] = UIVertex(vec3f(TL.x, TL.y, Position.z), vec2f(0.f, 0.f));
    VertData[1] = UIVertex(vec3f(TL.x, BR.y, Position.z), vec2f(0.f, 1.f));
    VertData[2] = UIVertex(vec3f(BR.x, BR.y, Position.z), vec2f(1.f, 1.f));
    VertData[3] = UIVertex(vec3f(BR.x, TL.y, Position.z), vec2f(1.f, 0.f));

    ++(RenderCmdCount[PanelIdx]);
    ParentID[ParentLayer++] = ID;

    // Add panel title as text
    MakeText(NULL, PanelTitle, Context->RenderResources.DefaultFont, Position + vec3i(5, 5, 1), col4f(0, 0, 0, 1), Size.x - 5);


    //printf("hprio : %d <= %d (hn:%d), pcount : %d", Hover.Priority, PanelOrder[PanelCount], HoverNext.Priority, PanelCount);
    if(HoverNext.Priority <= PanelOrder[PanelIdx])
    {
        if(PointInRectangle(vec2f(Input->MousePosX, Y - Input->MousePosY), TL, BR))
        {
            //printf(" check");
            HoverNext.ID = ID;
            HoverNext.Priority = PanelOrder[PanelIdx];
            strncpy(HoverTitleNext, PanelTitle, 64);
        }
    }
    //printf("\n");
}

void EndPanel()
{
    // TODO - Keep track of per-panel info, stacking, layout etc
    --ParentLayer;
    ++PanelCount;
}

static void UpdateOrder()
{

    // TODO - panel removal
    if(PanelCountNext < PanelCount)
    {

    }

#if 1
    if(MOUSE_HIT(Input->MouseLeft))
    {
        FocusNext = Hover;
        strncpy(FocusTitleNext, HoverTitleCurrent, 64);

        printf("hpr %d, po[hpr] %d\n", Hover.Priority, PanelOrder[Hover.Priority]);
        // reorder panel priority, only reorder if it changed by this click
        if(Hover.Priority > 0 && PanelOrder[Hover.Priority] < (PanelCount-1))
        {
            for(uint32 p = 1; p < PanelCount; ++p)
                --PanelOrder[p];
            PanelOrder[Hover.Priority] = PanelCount - 1;
            for(uint32 i = 0; i < PanelCount; ++i)
                printf("%d ", PanelOrder[i]);
            printf("\n");

            for(uint32 i = 0; i < PanelCount; ++i)
            {
                RenderOrder[PanelOrder[i]] = i;
            }
        }
    }
#endif

    // Look if all the panels asked this frame are already in the sort order
#if 0
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
    if(RenderCmdArena.Size > 0)
    {
        if(RenderCmdArena.Size < RenderCmdArena.Capacity/2)
        {
            OrderedRenderCmd = PushArenaData(&RenderCmdArena, RenderCmdArena.Size);

            render_info *Info = (render_info*)RenderCmdArena.BasePtr;
            while(Info && Info->ParentID == NULL)
            {
            }
        }
        else
        {
            printf("Error : UI Data Stack(%d) not large enough for UI rendering !\n", UI_STACK_SIZE);
        }
    }
#endif
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
    for(int p = PanelCount-1; p >=0; --p)
    {
        uint32 OrdPanelIdx = PanelOrder[p];
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
    glBindVertexArray(0);
}
}
