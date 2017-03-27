#ifndef RENDERER_H
#define RENDERER_H

#include "radar.h"

struct image
{
    uint8 *Buffer;
    int32 Width;
    int32 Height;
    int32 Channels;
};

uint32 BuildShader(char *VSPath, char *FSPath);
uint32 MakeVertexArrayObject();
uint32 AddIndexBufferObject(uint32 Usage, uint32 Size, void *Data);
uint32 AddVertexBufferObject(uint32 Attrib, uint32 AttribStride, uint32 Type, 
                             uint32 Usage, uint32 Size, void *Data);
#endif
