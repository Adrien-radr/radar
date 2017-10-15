#include "utils.h"

void MakeRelativePath(char *Dst, char const *Path, char const *Filename)
{
    strncpy(Dst, Path, MAX_PATH);
    strncat(Dst, Filename, MAX_PATH);
}

void *ReadFileContents(memory_arena *Arena, char const *Filename, int32 *FileSize)
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

