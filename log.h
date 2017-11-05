#ifndef LOG_H
#define LOG_H

#include "definitions.h"
#include <string>

namespace rlog
{
    enum log_level
    {
        LOG_INFO,
        LOG_ERROR,
        LOG_DEBUG
    };

    void Init(game_memory *Memory);
    void Destroy();
    void _Msg(log_level LogLevel, char const *File, int Line, char const *Fmt, ... );

#define LogInfo(...) do {\
    rlog::_Msg(rlog::LOG_INFO, __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)

#define LogError(...) do {\
    rlog::_Msg(rlog::LOG_INFO, __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)

#define LogDebug(...) do {\
    D(rlog::_Msg(rlog::LOG_DEBUG, __FILE__, __LINE__ ##__VA_ARGS__);)\
} while(0)
}

#endif
