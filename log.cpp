#include <cstdarg>
#include "log.h"

namespace rlog
{
    game_system *System;

    void Init(game_memory *Memory)
    {
        game_system *Sys = (game_system*)Memory->PermanentMemPool;
        System = Sys;
        System->ConsoleLog = (console_log*)PushArenaStruct(&Memory->SessionArena, console_log);
    }

    static void LogString(console_log *Log, char const *String)
    {
        memcpy(Log->MsgStack[Log->WriteIdx], String, CONSOLE_STRINGLEN);
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

    void _Msg(char const *File, int Line, char const *Fmt, ...)
    {
        char LocalBuf[256];
        va_list args;
        va_start(args, Fmt);
        vsnprintf(LocalBuf, 256, Fmt, args);
        va_end(args);

        printf("<%s:%d> %s\n", File, Line, LocalBuf);
    }

#if 0
    bool LogOpened = false;
    double LogTime = 0.0;
    path LogPath = "rengine.log";
    std::stringstream LogSS = std::stringstream();
    std::ofstream Log::log_file = std::ofstream();

#if 0
#ifdef RADAR_WIN32
void Sleep
#else
#ifdef RADAR_UNIX
#endif
#endif
#endif

void get_date_time( char *buffer, u32 bsize, const char *fmt )
{
	time_t ti = time( NULL );
	struct tm *lt = localtime( &ti );
	strftime( buffer, bsize, fmt, lt );
}

void Log::Init()
{
	if ( !log_opened )
	{
		log_opened = true;
		log_file = std::ofstream( log_name );
		if ( !log_file.is_open() )
		{
			std::cout << "Error while opening log " << log_name << std::endl;
			exit( 1 );
		}
		log_ss.str( std::string() );
		log_ss.setf( std::ios::fixed, std::ios::floatfield );
		log_ss.precision( 2 );

		char da[64], ti[64];
		get_date_time( da, 64, DEFAULT_DATE_FMT );
		get_date_time( ti, 64, DEFAULT_TIME_FMT );
		std::string dastr( da );
		std::string tistr( ti );
		std::string dt = dastr + " - " + tistr;
		LogInfo( "\t    Radar Log v", RADAR_MAJOR, ".", RADAR_MINOR, ".", RADAR_PATCH );
		LogInfo( "\t", dt.c_str() );
		LogInfo( "================================" );
	}
}

void Log::Close()
{
	if ( log_opened )
	{
		log_opened = false;
		log_file.close();
	}
}

double Log::get_engine_time()
{
	return glfwGetTime();
}
#endif
}
