#include <cstdarg>
#include <algorithm>
#include "log.h"
#include "utils.h"

namespace rlog
{
    game_system     *System;

    FILE static     *LogFile;
    double static   LogTime = 0.0;

    void Init(game_memory *Memory)
    {
        game_system *Sys = (game_system*)Memory->PermanentMemPool;
        System = Sys;
        System->ConsoleLog = (console_log*)PushArenaStruct(&Memory->SessionArena, console_log);

        path LogPath;
        MakeRelativePath(&Memory->ResourceHelper, LogPath, Memory->Config.LogFilename);
        LogFile = fopen(LogPath, "w");
        if(!LogFile)
        {
            printf("Error opening engine log file %s\n", LogPath);
        }

		char CurrDate[128], CurrTime[64];
		size_t WrittenChar = GetDateTime(CurrDate, 64, DEFAULT_DATE_FMT);
        CurrTime[0] = ' ';
        GetDateTime(CurrTime + 1, 63, DEFAULT_TIME_FMT);
        strncat(CurrDate + WrittenChar, CurrTime, 64);

        LogInfo("Radar Engine Log v%d.%d.%d", RADAR_MAJOR, RADAR_MINOR, RADAR_PATCH );
        LogInfo("%s", CurrDate);
        LogInfo( "========================" );
    }

    void Destroy()
    {
        if(LogFile)
        {
            char CurrTime[64];
            GetDateTime(CurrTime, 64, DEFAULT_TIME_FMT);
            LogInfo("Radar Engine Log End. %s\n", CurrTime);
            fclose(LogFile);
        }
    }

    static void ConsoleLog(console_log *Log, char const *String, size_t CharCount)
    {
        CharCount = std::min(size_t(CONSOLE_STRINGLEN-1), CharCount);
        strncpy(Log->MsgStack[Log->WriteIdx], String, CharCount);
        Log->MsgStack[Log->WriteIdx][CharCount] = 0; // EOL
        Log->WriteIdx = (Log->WriteIdx + 1) % CONSOLE_CAPACITY;

        if(Log->StringCount >= CONSOLE_CAPACITY)
        {
            Log->ReadIdx = (Log->ReadIdx + 1) % CONSOLE_CAPACITY;
        }
        else
        {
            Log->StringCount++;
        }
    }

    void _Msg(log_level LogLevel, char const *File, int Line, char const *Fmt, ...)
    {
        static char const *LogLevelStr[] = {
            "II",
            "EE",
            "DEB"
        };

        char LocalBuf[256];
        va_list args;
        va_start(args, Fmt);
        vsnprintf(LocalBuf, 256, Fmt, args);
        va_end(args);

        char Str[512];
        int CharCount = snprintf(Str, 512, "%s <%s:%d> %s", LogLevelStr[LogLevel], File, Line, LocalBuf);
        //int CharCount = snprintf(Str, 512, "%s <%s:%d> %s\n", LogLevelStr[LogLevel], File, Line, LocalBuf);

        // STD Output
        printf("%s\n", Str);

        // FILE Output
        fprintf(LogFile, "%s\n", Str);

        // CONSOLE Output
        ConsoleLog(System->ConsoleLog, Str, CharCount);
    }
}
