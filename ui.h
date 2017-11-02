#ifndef UI_H
#define UI_H

#include "context.h"

namespace ui {

    col4f const &GetColor(theme_color Col);
    font  *GetFont(theme_font Font);

    void Init(game_memory *Memory, game_context *Context);
    void BeginFrame(game_memory *Memory, game_input *Input);
    void BeginPanel(void* ID, char const *PanelTitle, vec3i Position, vec2i Size, col4f Color);
    void EndPanel();
    void Draw();

    // This version of MakeText is prefered to keep with the ui Coloring scheme
    // Use the 2nd version if a specific color is needed
    void MakeText(void *ID, char const *Text, theme_font Font, vec3i Position, theme_color Color, int MaxWidth);
    void MakeText(void *ID, char const *Text, theme_font Font, vec3i Position, col4f Color, int MaxWidth);

    void ReloadShaders(game_memory *Memory, game_context *Context);
}
#endif
