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

size_t UTF8_strnlen(char const *String, size_t MaxChar)
{
    size_t Len = 0;
    size_t i = 0;
    while(String[i] && i < MaxChar)
    {
        Len += (String[i++] & 0xc0) != 0x80;
    }

    return Len;
}
