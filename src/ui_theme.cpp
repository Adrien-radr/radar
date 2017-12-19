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
        col4f ConsoleBG;
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
    };

    ui_theme Theme;
    ui_theme DefaultTheme;

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
            case COLOR_CONSOLEBG : return Theme.ConsoleBG;
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

    static void ParseUIConfigRoot(ui_theme *DstTheme, cJSON *root, game_context *Context)
    {
        DstTheme->Red = JSON_Get(root, "Red", DefaultTheme.Red);
        DstTheme->Green = JSON_Get(root, "Green", DefaultTheme.Green);
        DstTheme->Blue = JSON_Get(root, "Blue", DefaultTheme.Blue);
        DstTheme->Black = JSON_Get(root, "Black", DefaultTheme.Black);
        DstTheme->White = JSON_Get(root, "White", DefaultTheme.White);

        DstTheme->PanelBG = JSON_Get(root, "PanelBG", DefaultTheme.PanelBG);
        DstTheme->PanelFG = JSON_Get(root, "PanelFG", DefaultTheme.PanelFG);
        DstTheme->TitlebarBG = JSON_Get(root, "TitlebarBG", DefaultTheme.TitlebarBG);
        DstTheme->BorderBG = JSON_Get(root, "BorderBG", DefaultTheme.BorderBG);
        DstTheme->ConsoleBG = JSON_Get(root, "ConsoleBG", DefaultTheme.ConsoleBG);
        DstTheme->ConsoleFG = JSON_Get(root, "ConsoleFG", DefaultTheme.ConsoleFG);
        DstTheme->DebugFG = JSON_Get(root, "DebugFG", DefaultTheme.DebugFG);
        DstTheme->SliderBG = JSON_Get(root, "SliderBG", DefaultTheme.SliderBG);
        DstTheme->SliderFG = JSON_Get(root, "SliderFG", DefaultTheme.SliderFG);
        DstTheme->ButtonBG = JSON_Get(root, "ButtonBG", DefaultTheme.ButtonBG);
        DstTheme->ButtonPressedBG = JSON_Get(root, "ButtonPressedBG", DefaultTheme.ButtonPressedBG);
        DstTheme->ProgressbarBG = JSON_Get(root, "ProgressbarBG", DefaultTheme.ProgressbarBG);
        DstTheme->ProgressbarFG = JSON_Get(root, "ProgressbarFG", DefaultTheme.ProgressbarFG);

        DstTheme->DefaultFont = ParseConfigFont(root, Context, "DefaultFont", 32, 127);
        DstTheme->ConsoleFont = ParseConfigFont(root, Context, "ConsoleFont", 32, 127);
        DstTheme->AwesomeFont = ParseConfigFont(root, Context, "AwesomeFont", ICON_MIN_FA, 1+ICON_MAX_FA);
    }

    static void ParseDefaultUIConfig(game_memory *Memory, game_context *Context)
    {
        path DefaultConfigPath;
        MakeRelativePath(&Memory->ResourceHelper, DefaultConfigPath, "default_ui_config.json");

        // If the default config doesnt exist, just crash, someone has been stupid
        if(!DiskFileExists(DefaultConfigPath))
        {
            printf("Fatal Error : Default UI Config file bin/default_ui_config.json doesn't exist.\n");
            exit(1);
        }

        void *Content = ReadFileContents(&Memory->ScratchArena, DefaultConfigPath, 0);
        if(Content)
        {
            cJSON *root = cJSON_Parse((char*)Content);
            if(root)
            {
                ParseUIConfigRoot(&DefaultTheme, root, Context);
            }
            else
            {
                printf("Fatal Error parsing UI Config File (%s) as JSON.\n", DefaultConfigPath);
                exit(1);
            }
        }
        else
        {
            Assert(false); // should never happen because of the file check
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
                ParseUIConfigRoot(&Theme, root, Context);
            }
            else
            {
                printf("Error parsing UI Config File (%s) as JSON. Using Default Theme.\n", ConfigPath);
                Theme = DefaultTheme;
            }
        }
        else
        {
            printf("Generating UI theme from Default Theme...\n");

            path DefaultConfigPath;
            MakeRelativePath(&Memory->ResourceHelper, DefaultConfigPath, "default_ui_config.json");
            path PersonalConfigPath;
            MakeRelativePath(&Memory->ResourceHelper, PersonalConfigPath, "ui_config.json");
            DiskFileCopy(PersonalConfigPath, DefaultConfigPath);

            Theme = DefaultTheme;
        }
    }

}
