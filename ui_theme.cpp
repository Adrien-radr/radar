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
        col4f TitlebarBG;
        col4f BorderBG;
        col4f ConsoleFG;
        col4f SliderBG;
        col4f SliderFG;
        col4f ButtonBG;
        col4f ButtonPressedBG;
        col4f ProgressbarBG;
        col4f ProgressbarFG;

        col4f DebugFG;

        font  *DefaultFont;
        font  *ConsoleFont;
        font  *AwesomeFont;
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
            case COLOR_TITLEBARBG : return Theme.TitlebarBG;
            case COLOR_BORDERBG : return Theme.BorderBG;
            case COLOR_CONSOLEFG : return Theme.ConsoleFG;
            case COLOR_DEBUGFG : return Theme.DebugFG;
            case COLOR_SLIDERBG : return Theme.SliderBG;
            case COLOR_SLIDERFG : return Theme.SliderFG;
            case COLOR_BUTTONBG : return Theme.ButtonBG;
            case COLOR_BUTTONPRESSEDBG : return Theme.ButtonPressedBG;
            case COLOR_PROGRESSBARBG : return Theme.ProgressbarBG;
            case COLOR_PROGRESSBARFG : return Theme.ProgressbarFG;
            default : return Theme.White;
        }
    }

    font *GetFont(theme_font Font)
    {
        switch(Font)
        {
            case FONT_DEFAULT : return Theme.DefaultFont;
            case FONT_CONSOLE : return Theme.ConsoleFont;
            case FONT_AWESOME : return Theme.AwesomeFont;
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
        Theme.TitlebarBG = col4f(1, 1, 1, 0.1);
        Theme.BorderBG = col4f(1, 1, 1, 0.2);
        Theme.DebugFG = col4f(1, 0, 0, 1);
        Theme.ConsoleFG = col4f(1, 1, 1, 0.9);
        Theme.SliderBG = col4f(1, 1, 1, 0.2);
        Theme.SliderFG = col4f(0, 0, 0, 0.6);
        Theme.ButtonBG = col4f(1, 1, 1, 0.1);
        Theme.ButtonPressedBG = col4f(1, 1, 1, 0.1);
        Theme.ProgressbarBG = col4f(0, 0, 0, 0.2);
        Theme.ProgressbarFG = col4f(1, 1, 1, 0.1);

        Theme.DefaultFont = ResourceLoadFont(&Context->RenderResources, "data/DroidSansMonoSlashed.ttf", 13);
        Theme.ConsoleFont = ResourceLoadFont(&Context->RenderResources, "data/DroidSansMonoSlashed.ttf", 13);
        Theme.AwesomeFont = ResourceLoadFont(&Context->RenderResources, "data/fontawesome.ttf", 24);
    }

    static font *ParseConfigFont(cJSON *root, game_context *Context, char const *Name, int c0, int cn)
    {
        cJSON *FontInfo = cJSON_GetObjectItem(root, Name);
        if(FontInfo && cJSON_GetArraySize(FontInfo) == 2)
        {
            path FontPath;
            strncpy(FontPath, cJSON_GetArrayItem(FontInfo, 0)->valuestring, MAX_PATH);
            int FontSize = cJSON_GetArrayItem(FontInfo, 1)->valueint;
            return ResourceLoadFont(&Context->RenderResources, FontPath, FontSize, c0, cn);
        }
        else
        {
            printf("Error loading UI Theme Font %s, loading default DroidSansMono instead.\n", Name);
            return ResourceLoadFont(&Context->RenderResources, "data/DroidSansMonoSlashed.ttf", 13, 32, 127);
        }
    }

    static void ParseUIConfig(game_memory *Memory, game_context *Context, path const ConfigPath)
    {
        void *Content = ReadFileContents(&Memory->ScratchArena, ConfigPath, 0);
        if(Content)
        {
            cJSON *root = cJSON_Parse((char*)Content);
            if(root)
            {
                // TODO - have a default storage in static here for all of those so that we dont repeat here + MakeDefault
                Theme.Red = JSON_Get(root, "Red", col4f(1, 0, 0, 1));
                Theme.Green = JSON_Get(root, "Green", col4f(0, 1, 0, 1));
                Theme.Blue = JSON_Get(root, "Blue", col4f(0, 0, 1, 1));
                Theme.Black = JSON_Get(root, "Black", col4f(0, 0, 0, 1));
                Theme.White = JSON_Get(root, "White", col4f(1, 1, 1, 1));

                Theme.PanelBG = JSON_Get(root, "PanelBG", col4f(0, 0, 0, 0.9));
                Theme.PanelFG = JSON_Get(root, "PanelFG", col4f(1, 1, 1, 1));
                Theme.TitlebarBG = JSON_Get(root, "TitlebarBG", col4f(1, 1, 1, 0.1));
                Theme.BorderBG = JSON_Get(root, "BorderBG", col4f(1, 1, 1, 0.2));
                Theme.ConsoleFG = JSON_Get(root, "ConsoleFG", col4f(1, 1, 1, 0.9));
                Theme.DebugFG = JSON_Get(root, "DebugFG", col4f(1, 0, 0, 1));
                Theme.SliderBG = JSON_Get(root, "SliderBG", col4f(0, 0, 0, 0.2));
                Theme.SliderFG = JSON_Get(root, "SliderFG", col4f(1, 1, 1, 0.1));
                Theme.ButtonBG = JSON_Get(root, "ButtonBG", col4f(1, 1, 1, 0.1));
                Theme.ButtonPressedBG = JSON_Get(root, "ButtonPressedBG", col4f(1, 1, 1, 0.1));
                Theme.ProgressbarBG = JSON_Get(root, "ProgressbarBG", col4f(1, 1, 1, 0.2));
                Theme.ProgressbarFG = JSON_Get(root, "ProgressbarFG", col4f(0, 0, 0, 0.6));

                Theme.DefaultFont = ParseConfigFont(root, Context, "DefaultFont", 32, 127);
                Theme.ConsoleFont = ParseConfigFont(root, Context, "ConsoleFont", 32, 127);
                Theme.AwesomeFont = ParseConfigFont(root, Context, "AwesomeFont", ICON_MIN_FA, 1+ICON_MAX_FA);
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
