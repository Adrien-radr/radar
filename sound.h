#ifndef SOUND_H
#define SOUND_H

#include "radar_common.h"

bool InitAL();
void DestroyAL();

struct tmp_sound_data
{
    bool ReloadSoundBuffer;
    uint32 SoundBufferSize;
    uint16 SoundBuffer[Megabytes(1)];
};

#endif
