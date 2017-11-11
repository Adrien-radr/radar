#include <ctime>
#include "utils.h"

void MakeRelativePath(resource_helper *RH, path Dst, path const Filename)
{
    strncpy(Dst, RH->ExecutablePath, MAX_PATH);
    strncat(Dst, Filename, MAX_PATH);
}

void *ReadFileContents(memory_arena *Arena, path const Filename, int32 *FileSize)
{
    char *Contents = NULL;
    FILE *fp = fopen(Filename, "rb");

    if(fp)
    {
        if(0 == fseek(fp, 0, SEEK_END))
        {
            int32 Size = ftell(fp);
            rewind(fp);
            Contents = (char*)PushArenaData(Arena, Size+1);
            fread(Contents, Size, 1, fp);
            Contents[Size] = 0;
            if(FileSize)
            {
                *FileSize = Size+1;
            }
        }
        else
        {
            printf("File Open Error : fseek not 0.\n");
        }
        fclose(fp);
    }
    else
    {
        printf("Coudln't open file %s.\n", Filename);
    }

    return (void*)Contents;
}

size_t GetDateTime(char *Dst, size_t DstSize, char const *Fmt)
{
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    return strftime(Dst, DstSize, Fmt, timeinfo);
}

size_t UTF8_strnlen(char const *Str, size_t MaxChar)
{
    size_t Len = 0;
    size_t i = 0;
    while(Str[i] && i < MaxChar)
    {
        Len += (Str[i++] & 0xc0) != 0x80;
    }

    return Len;
}

uint16 UTF8CharToInt(char const *Str, size_t *CharAdvance)
{
    uint16 uni = 0;
    uint8 BaseCH = Str[0];

    if(BaseCH <= 0x7F)
    {
        uni = BaseCH;
        *CharAdvance = 0;
    }
    else if(BaseCH <= 0xBF)
    {
        return 0; // Not an UTF-8 char
    }
    else if(BaseCH <= 0xDF)
    {
        uni = BaseCH & 0x1F;
        *CharAdvance = 1;
    }
    else if(BaseCH <= 0xEF)
    {
        uni = BaseCH & 0x0F;
        *CharAdvance = 2;
    }
    else if(BaseCH <= 0xF7)
    {
        uni = BaseCH & 0x07;
        *CharAdvance = 3;
    }
    else
    {
        return 0; // Not an UTF-8 Char
    }

    *CharAdvance += 1; // for the leading char

    for (size_t j = 1; j < *CharAdvance; ++j)
    {
        uint8 ch = Str[j];
        if (ch < 0x80 || ch > 0xBF)
            return 0; // Not an UTF-8
        uni <<= 6;
        uni += ch & 0x3F;
    }

    return uni;
}
