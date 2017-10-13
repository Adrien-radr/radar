#ifndef UI_H
#define UI_H

// TODO - See if 256 is enough for more one liner ui strings
#define CONSOLE_CAPACITY 8
#define CONSOLE_STRINGLEN 256
#define UI_STRINGLEN 256
#define UI_MAXSTACKOBJECT 256

typedef char console_log_string[CONSOLE_STRINGLEN];
struct console_log
{
    console_log_string MsgStack[CONSOLE_CAPACITY];
    uint32 WriteIdx;
    uint32 ReadIdx;
    uint32 StringCount;
};

enum ui_font
{
    DEFAULT_FONT
};

struct ui_text_line
{
    char    String[UI_STRINGLEN]; 
    ui_font Font;
    vec3f   Position;
    col4f   Color;
};

struct ui_frame_stack
{
    ui_text_line TextLines[UI_MAXSTACKOBJECT];
    uint32 TextLineCount;
};

#endif
