#ifndef UI_H
#define UI_H

#include "context.h"
#include "fontawesome.h"

// NOTE - Maybe parameterize this somewhere
#define UI_TITLEBAR_HEIGHT 20
#define UI_BORDER_WIDTH 1
#define UI_MARGIN_WIDTH 4

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

    bool HasFocus();

    // DecorationFlags is of the type decoration_flag from definitions.h
    void BeginPanel(uint32* ID, char const *PanelTitle, vec3i *Position, vec2i *Size, theme_color Color, 
                    uint32 DecorationFlags = DECORATION_TITLEBAR | DECORATION_RESIZE | DECORATION_BORDER);
    void EndPanel();

    /// ID's value will be the slider relative position
    void MakeSlider(real32 *ID, real32 MinVal, real32 MaxVal);

    /// ID's value is the progressbar current position in [0, MaxVal]
    void MakeProgressbar(real32 *ID, real32 MaxVal, vec2i const &PositionOffset, vec2i const &Size);

    /// ID's value is the image current texture scale
    void MakeImage(real32 *ID, uint32 TextureID, vec2f *TexOffset, vec2i const &Size, bool FlipY);

    bool MakeButton(uint32 *ID, char const *ButtonText, theme_font Font, vec2i const &PositionOffset, vec2i const &Size,
                    real32 FontScale = 1.f, int32 DecorationFlags = DECORATION_MARGIN | DECORATION_BORDER);

    void MakeText(void *ID, char const *Text, theme_font FontStyle, vec2i PositionOffset, theme_color Color, real32 FontScale = 1.f, int MaxWidth = 2000);
    void MakeText(void *ID, char const *Text, theme_font FontStyle, vec2i PositionOffset, col4f const &Color, real32 FontScale = 1.f, int MaxWidth = 2000);
}
#endif
