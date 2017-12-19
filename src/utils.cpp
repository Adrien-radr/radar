#include <ctime>
#include "utils.h"

void MakeRelativePath(resource_helper *RH, path Dst, path const Filename)
{
    strncpy(Dst, RH->ExecutablePath, MAX_PATH);
    strncat(Dst, Filename, MAX_PATH);
}

bool DiskFileExists(path const Filename)
{
    FILE *fp = fopen(Filename, "rb");
    if(fp)
    {
        fclose(fp);
        return true;
    }
    return false;
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
            size_t Read = fread(Contents, Size, 1, fp);
            if(Read != 1)
            {
                LogError("File Open Error [%s] : Reading error or EOF reached.", Filename);
            }
            Contents[Size] = 0;
            if(FileSize)
            {
                *FileSize = Size+1;
            }
        }
        else
        {
            LogError("File Open Error [%s] : fseek not 0.", Filename);
        }
        fclose(fp);
    }
    else
    {
        LogError("File Open Error [%s] : Couldn't open file.", Filename);
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

int    UTF8CharCount(char const *Str, uint16 *OutUnicode)
{
    if(strlen(Str) == 0) return -1;

    int CharCount, Unicode;

    uint8 BaseCH = Str[0];
    if(BaseCH <= 0x7F) // <128, normal char
    {
        Unicode = BaseCH;
        CharCount = 1;
    }
    else if(BaseCH <= 0xBF)
    {
        return -1; // Not an UTF-8 char
    }
    else if(BaseCH <= 0xDF) // <223, 1 more char follows
    {
        Unicode = BaseCH & 0x1F;
        CharCount = 2;
    }
    else if(BaseCH <= 0xEF) // <239, 2 more char follows
    {
        Unicode = BaseCH & 0x0F;
        CharCount = 3;
    }
    else if(BaseCH <= 0xF7) // <247, 3 more char follows
    {
        Unicode = BaseCH & 0x07;
        CharCount = 4;
    }
    else
    {
        return -1; // Not an UTF-8 Char
    }

    if(OutUnicode)
        *OutUnicode = Unicode;

    return CharCount;
}

size_t UTF8Len(char const *Str, size_t MaxChar)
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
    uint16 Unicode = 0;
    uint8 BaseCH = Str[0];

    int Count = UTF8CharCount(Str, &Unicode);
    if(Count < 0)
        return 0;

    *CharAdvance = Count; // for the leading char

    for (size_t j = 1; j < *CharAdvance; ++j)
    {
        uint8 ch = Str[j];
        if (ch < 0x80 || ch > 0xBF)
            return 0; // Not an UTF-8
        Unicode <<= 6;
        Unicode += ch & 0x3F;
    }

    return Unicode;
}
