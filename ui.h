#ifndef UI_H
#define UI_H

#include "context.h"

namespace ui {
    col4f const &GetColor(theme_color Col);
    font  *GetFont(theme_font Font);

    void Init(game_memory *Memory, game_context *Context);
    void BeginFrame(game_memory *Memory, game_input *Input);

    /// ID's value must be initialized to 0 in the calling code.
    /// This insure that BeginPanel will give the focus to this panel automatically in an "init stage"
    /// ID's value will be set at the "init stage" to the Idx value of this panel in the UI priority array
    void BeginPanel(uint32* ID, char const *PanelTitle, vec3i *Position, vec2i Size, decoration_flag Flags);
    void EndPanel();
    void Draw();

    // This version of MakeText is prefered to keep with the ui Coloring scheme
    // Use the 2nd version if a specific color is needed
    void MakeText(void *ID, char const *Text, theme_font Font, vec3i Position, theme_color Color, int MaxWidth);
    void MakeText(void *ID, char const *Text, theme_font Font, vec3i Position, col4f Color, int MaxWidth);

    void ReloadShaders(game_memory *Memory, game_context *Context);
}
#endif
