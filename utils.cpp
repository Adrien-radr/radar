path ExecutableFullPath;

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

///////////////////////////////////////////////////////////////
// Sampling procedures 

real32 Halton2(uint32 Index) {
    real32 f = 0.5;
    uint32 i = Index;

    real32 Result = 0.0;

    while (i > 0) {
        Result += f*(i & 1);
        i = i >> 1;
        f *= 0.5;
    }

    return Result;
}

real32 Halton3(const uint32 Index) {
    real32 Result = 0.0;
    real32 f = 1.0 / 3.0;
    uint32 i = Index;
    while (i > 0) {
        Result = Result + f*(i % 3);
        f = f / 3.0;
        i = i / 3;
    }

    return Result;
}

real32 Halton5(const uint32 Index) {
    real32 Result = 0.0;
    real32 f = 1.0f / 5.0;
    uint32 i = Index;
    while (i > 0) {
        Result = Result + f*(i % 5);
        f = f / 5.0;
        i = i / 5;
    }

    return Result;
}

real32 VanDerCorput(uint32 bits)
{ // NOTE - from [Warren01]
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2f SampleHammersley(uint32 i, real32 InverseN)
{
    return vec2f(i * InverseN, VanDerCorput(i));
}

vec3f SampleHemisphereUniform(real32 u, real32 v)
{
    real32 Phi = v * M_TWO_PI;
    real32 CosTheta = 1.f - u;
    real32 SinTheta = sqrtf(1.f - CosTheta * CosTheta);
    return vec3f(cos(Phi) * SinTheta, CosTheta, sin(Phi) * SinTheta);
}

vec3f SampleHemisphereCosine(real32 u, real32 v)
{
    real32 Phi = v * M_TWO_PI;
    real32 CosTheta = sqrtf(1.f - u);
    real32 SinTheta = sqrtf(1.f - CosTheta * CosTheta);
    return vec3f(cos(Phi) * SinTheta, CosTheta, sin(Phi) * SinTheta);
}
