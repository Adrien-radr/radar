#include "sound.h"

#include "AL/al.h"
#include "AL/alc.h"

namespace sound {
bool Init()
{
    ALCdevice *ALDevice = alcOpenDevice(NULL);

    bool ALValid = (NULL != ALDevice);
    if(ALValid)
    {
        ALCcontext *ALContext = alcCreateContext(ALDevice, NULL);
        alcMakeContextCurrent(ALContext);
        alGetError(); // Clear Error Stack
    }
    else
    {
        printf("Couldn't init OpenAL.\n");
    }
    
    return ALValid;
}

void Destroy()
{
    ALCcontext *ALContext = alcGetCurrentContext();
    if(ALContext)
    {
        ALCdevice *ALDevice = alcGetContextsDevice(ALContext);
        alcMakeContextCurrent(NULL);
        alcDestroyContext(ALContext);
        alcCloseDevice(ALDevice);
    }
}

bool CheckALError()
{
    ALenum Error = alGetError();
    if(Error != AL_NO_ERROR)
    {
        char ErrorMsg[128];
        switch(Error)
        {
        case AL_INVALID_NAME:
        {
            snprintf(ErrorMsg, 128, "Bad Name ID passed to AL."); 
        }break;
        case AL_INVALID_ENUM:
        {
            snprintf(ErrorMsg, 128, "Invalid Enum passed to AL."); 
        }break;
        case AL_INVALID_VALUE:
        {
            snprintf(ErrorMsg, 128, "Invalid Value passed to AL."); 
        }break;
        case AL_INVALID_OPERATION:
        {
            snprintf(ErrorMsg, 128, "Invalid Operation requested to AL."); 
        }break;
        case AL_OUT_OF_MEMORY:
        {
            snprintf(ErrorMsg, 128, "Out of Memory."); 
        }break;
        }

        printf("OpenAL Error : %s\n", ErrorMsg);
        return false;
    }
    return true;
}

bool TempPrepareSound(ALuint *Buffer, ALuint *Source)
{
    // Generate Buffers
    alGenBuffers(1, Buffer);
    if(!CheckALError()) return false;

    // Generate Sources
    alGenSources(1, Source);
    if(!CheckALError()) return false;

    // Attach Buffer to Source
    alSourcei(*Source, AL_LOOPING, AL_TRUE);
    if(!CheckALError()) return false;

    return true;
}
}
