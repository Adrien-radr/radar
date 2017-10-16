#ifndef UI_H
#define UI_H

#include "context.h"

void uiInit(game_context *Context);
void uiBeginFrame(game_memory *Memory, game_input *Input);
void uiBeginPanel(char const *PanelTitle, vec3i Position, vec2i Size, col4f Color);
void uiEndPanel();
void uiDraw();
void uiMakeText(char const *Text, font *Font, vec3i Position, col4f Color, int MaxWidth);
void uiReloadShaders(game_memory *Memory, game_context *Context, path ExecFullPath);
#endif
