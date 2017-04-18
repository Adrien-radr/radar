#ifndef UI_CPP
#define UI_CPP

uint32 ProgramUI;

void ReloadUIShaders(game_memory *Memory, path ExecFullPath)
{
    path VSPath, FSPath;
    MakeRelativePath(VSPath, ExecFullPath, "data/shaders/ui_vert.glsl");
    MakeRelativePath(FSPath, ExecFullPath, "data/shaders/ui_frag.glsl");
    ProgramUI = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProgramUI);
    SendInt(glGetUniformLocation(ProgramUI, "Texture0"), 0);
}

#endif
