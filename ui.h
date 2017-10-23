#ifndef UI_H
#define UI_H

#include "context.h"

namespace ui {
    void Init(game_context *Context);
    void BeginFrame(game_memory *Memory, game_input *Input);
    void BeginPanel(char const *PanelTitle, vec3i Position, vec2i Size, col4f Color);
    void EndPanel();
    void Draw();
    void MakeText(char const *Text, font *Font, vec3i Position, col4f Color, int MaxWidth);
    void ReloadShaders(game_memory *Memory, game_context *Context);
}
#endif
