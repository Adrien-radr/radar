#ifndef LOG_H
#define LOG_H

#include "definitions.h"
#include <string>

namespace rlog
{
    void Init(game_memory *Memory);
    void Destroy();
    void _Msg(char const *File, int Line, char const *Fmt, ... );

#if 0
#define LogInfo(...) do {\
	Log::Info(__FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)

#define LogErr(...) do {\
	Log::Err(__FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)

#define LogDebug(...) do {\
	D(LogInfo("[DEBUG] ", ##__VA_ARGS__);)\
} while(0)
#endif

#define LogMsg(...) do {\
    rlog::_Msg(__FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
}

#endif
