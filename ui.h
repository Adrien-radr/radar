#ifndef UI_H
#define UI_H

#include "context.h"
#include "fontawesome.h"

/// Panels are created by giving a permanent uint32 ID pointer
/// This ID should be set to 0 by the caller at first, to signify an uninitialized state.
/// This ID's value will be filled by the Panel Index in the internal panel focus/sorting system
/// This ID once set shouldn't be modified to another value, but can be reset to 0 if the panel has to be
/// reinitialized for some reason.

namespace ui {
    col4f const &GetColor(theme_color Col);
    font  *GetFont(theme_font Font);

    void Init(game_memory *Memory, game_context *Context);
    void ReloadShaders(game_memory *Memory, game_context *Context);
    void BeginFrame(game_memory *Memory, game_input *Input);
    void Draw();

    // DecorationFlags is of the type decoration_flag from definitions.h
    void BeginPanel(uint32* ID, char const *PanelTitle, vec3i *Position, vec2i *Size, uint32 DecorationFlags);
    void EndPanel();

    /// ID's value will be the slider relative position
    void MakeSlider(real32 *ID, real32 MinVal, real32 MaxVal);

    void MakeImage(real32 *ID, uint32 TextureID, vec2f *TexOffset, vec2i const &Size, bool FlipY);
    bool MakeButton(uint32 *ID, char *ButtonText, vec2i const &PositionOffset, vec2i const &Size);

    // This version of MakeText is prefered to keep with the ui Coloring scheme
    // Use the 2nd version if a specific color is needed
    //void MakeText(void *ID, char const *Text, theme_font Font, vec2i PositionOffset, theme_color Color, int MaxWidth);
    //void MakeText(void *ID, char const *Text, theme_font Font, vec2i PositionOffset, col4f Color, int MaxWidth);

    // UTF8 Version
    void MakeText(void *ID, char const *Text, theme_font FontStyle, vec2i PositionOffset, theme_color Color, int MaxWidth);
    void MakeText(void *ID, char const *Text, theme_font FontStyle, vec2i PositionOffset, col4f const &Color, int MaxWidth);
}
#endif
