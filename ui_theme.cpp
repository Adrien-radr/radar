namespace ui
{
    struct ui_theme
    {
        col4f Red;
        col4f Green;
        col4f Blue;
        col4f Black;
        col4f White;

        col4f PanelBG;
        col4f PanelFG;

        col4f DebugFG;

        font  *DefaultFont;
    } Theme;

    col4f const &GetColor(theme_color Col)
    {
        switch(Col)
        {
            case COLOR_RED : return Theme.Red;
            case COLOR_GREEN : return Theme.Green;
            case COLOR_BLUE : return Theme.Blue;
            case COLOR_WHITE : return Theme.White;
            case COLOR_BLACK : return Theme.Black;
            case COLOR_PANELBG : return Theme.PanelBG;
            case COLOR_PANELFG : return Theme.PanelFG;
            case COLOR_DEBUGFG : return Theme.DebugFG;
            default : return Theme.White;
        }
    }

    font *GetFont(theme_font Font)
    {
        switch(Font)
        {
            case FONT_DEFAULT : return Theme.DefaultFont;
            default : return Theme.DefaultFont;
        }
    }

    static void MakeDefaultConfig(game_context *Context)
    {
        Theme.Red = col4f(1, 0, 0, 1);
        Theme.Green = col4f(0, 1, 0, 1);
        Theme.Blue = col4f(0, 0, 1, 1);
        Theme.Black = col4f(0, 0, 0, 1);
        Theme.White = col4f(1, 1, 1, 1);

        Theme.PanelBG = col4f(0, 0, 0, 0.9);
        Theme.PanelFG = col4f(1, 1, 1, 1);
        Theme.DebugFG = col4f(1, 0, 0, 1);

#ifdef RADAR_WIN32
        Theme.DefaultFont = ResourceLoadFont(&Context->RenderResources, "C:/Windows/Fonts/dejavusansmono.ttf", 14);
#else
        Theme.DefaultFont = ResourceLoadFont(&Context->RenderResources, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
#endif
    }

    static void ParseUIConfig(game_memory *Memory, game_context *Context, path const ConfigPath)
    {
        void *Content = ReadFileContents(&Memory->ScratchArena, ConfigPath, 0);
        if(Content)
        {
            cJSON *root = cJSON_Parse((char*)Content);
            if(root)
            {
                Theme.Red = JSON_Get(root, "PanelBG", col4f(1, 0, 0, 1));
                Theme.Green = JSON_Get(root, "Green", col4f(0, 1, 0, 1));
                Theme.Blue = JSON_Get(root, "Blue", col4f(0, 0, 1, 1));
                Theme.Black = JSON_Get(root, "Black", col4f(0, 0, 0, 1));
                Theme.White = JSON_Get(root, "White", col4f(1, 1, 1, 1));

                Theme.PanelBG = JSON_Get(root, "PanelBG", col4f(0, 0, 0, 0.9));
                Theme.PanelFG = JSON_Get(root, "PanelFG", col4f(1, 1, 1, 1));
                Theme.DebugFG = JSON_Get(root, "DebugFG", col4f(1, 0, 0, 1));

                // TODO - Load default font with font path in the .json
                cJSON *FontInfo = cJSON_GetObjectItem(root, "DefaultFont");
                if(FontInfo && cJSON_GetArraySize(FontInfo) == 2)
                {
                    path FontPath;
                    strncpy(FontPath, cJSON_GetArrayItem(FontInfo, 0)->valuestring, MAX_PATH);
                    int FontSize = cJSON_GetArrayItem(FontInfo, 1)->valueint;
                    Theme.DefaultFont = ResourceLoadFont(&Context->RenderResources, FontPath, FontSize);
                }
                else
                {
                    printf("Error loading UI Theme Font, loading default DejaVuSansMono.\n");
#ifdef RADAR_WIN32
                    Theme.DefaultFont = ResourceLoadFont(&Context->RenderResources, "C:/Windows/Fonts/dejavusansmono.ttf", 14);
#else
                    Theme.DefaultFont = ResourceLoadFont(&Context->RenderResources, "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14);
#endif
                }
            }
            else
            {
                printf("Error parsing UI Config File (%s) as JSON.\n", ConfigPath);
            }
        }
        else
        {
            printf("No bin/ui_config.json found. Generating default UI theme.\n");
            MakeDefaultConfig(Context);
        }
    }

}
