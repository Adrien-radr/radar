#ifndef UTILS_H
#define UTILS_H

#include "definitions.h"
#include "cJSON.h"

void *ReadFileContents(memory_arena *Arena, path const Filename, int *FileSize);
void MakeRelativePath(resource_helper *RH, path Dst, path const Filename);

template<typename T>
inline T JSON_Get(cJSON *Root, char const *ValueName, T const &DefaultValue)
{
}

template<>
inline int JSON_Get(cJSON *Root, char const *ValueName, int const &DefaultValue)
{
    cJSON *Obj = cJSON_GetObjectItem(Root, ValueName);
    if(Obj)
        return Obj->valueint;
    return DefaultValue;
}

template<>
inline double JSON_Get(cJSON *Root, char const *ValueName, double const &DefaultValue)
{
    cJSON *Obj = cJSON_GetObjectItem(Root, ValueName);
    if(Obj)
        return Obj->valuedouble;
    return DefaultValue;
}

template<>
inline vec3f JSON_Get(cJSON *Root, char const *ValueName, vec3f const &DefaultValue)
{
    cJSON *Obj = cJSON_GetObjectItem(Root, ValueName);
    if(Obj && cJSON_GetArraySize(Obj) == 3)
    {
        vec3f Ret;
        Ret.x = (real32)cJSON_GetArrayItem(Obj, 0)->valuedouble;
        Ret.y = (real32)cJSON_GetArrayItem(Obj, 1)->valuedouble;
        Ret.z = (real32)cJSON_GetArrayItem(Obj, 2)->valuedouble;
        return Ret;
    }

    return DefaultValue;
}

template<>
inline vec4f JSON_Get(cJSON *Root, char const *ValueName, vec4f const &DefaultValue)
{
    cJSON *Obj = cJSON_GetObjectItem(Root, ValueName);
    if(Obj && cJSON_GetArraySize(Obj) == 4)
    {
        vec4f Ret;
        Ret.x = (real32)cJSON_GetArrayItem(Obj, 0)->valuedouble;
        Ret.y = (real32)cJSON_GetArrayItem(Obj, 1)->valuedouble;
        Ret.z = (real32)cJSON_GetArrayItem(Obj, 2)->valuedouble;
        Ret.w = (real32)cJSON_GetArrayItem(Obj, 3)->valuedouble;
        return Ret;
    }

    return DefaultValue;
}

template<>
inline col4f JSON_Get(cJSON *Root, char const *ValueName, col4f const &DefaultValue)
{
    cJSON *Obj = cJSON_GetObjectItem(Root, ValueName);
    if(Obj && cJSON_GetArraySize(Obj) == 4)
    {
        col4f Ret;
        Ret.x = (real32)cJSON_GetArrayItem(Obj, 0)->valuedouble;
        Ret.y = (real32)cJSON_GetArrayItem(Obj, 1)->valuedouble;
        Ret.z = (real32)cJSON_GetArrayItem(Obj, 2)->valuedouble;
        Ret.w = (real32)cJSON_GetArrayItem(Obj, 3)->valuedouble;
        return Ret;
    }

    return DefaultValue;
}

#endif
