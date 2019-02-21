#include "water.h"
#include "rf/context.h"
#include "rf/utils.h"
#include "Game/sun.h"

// NOTE - Tmp storage here
// Beaufort Level : WidthScale, WaveScale, Choppiness
// Beaufort     1 :          3,      0.05,      0.005
// Beaufort     3 :          3,       0.1,        0.1
// Beaufort     7 :          3,       0.5,        0.5
// Beaufort    12 :          3,       1.0,        1.0
real32 BeaufortParams[][3] = {
    { 3.f, 0.05f, 0.005f },
    { 3.f,  0.1f,   0.1f },
    { 3.f,  0.5f,   0.5f },
    { 3.f,  1.0f,   1.0f }
};

struct wave_vector
{
    complex H; // Wave Height
    vec2f   D; // Wave XZ Displacement
    vec3f   N; // Wave Normal
};

// TODO - Should return a vec2f, with complex having vec2f cast
// TODO - Use precomputed and better random variables, not rand()
complex GaussianRandomVariable()
{
    float U, V, W;
    do {
        U = 2.f * rand()/(real32)RAND_MAX - 1.f;
        V = 2.f * rand()/(real32)RAND_MAX - 1.f;
        W = Square(U) + Square(V);
    } while(W >= 1.f);
    W = sqrtf((-2.f * logf(W)) / W);
    return complex(U * W, V * W);
}

real32 Phillips(water::beaufort_state *State, int n_prime, int m_prime)
{
    vec2f K(M_PI * (2.f * n_prime - water::system::WaterN) / State->Width,
            M_PI * (2.f * m_prime - water::system::WaterN) / State->Width);
    real32 KLen = Length(K);
    if(KLen < 1e-6f) return 0.f;

    real32 KLen2 = Square(KLen);
    real32 KLen4 = Square(KLen2);

    vec2f UnitK = Normalize(K);
    vec2f UnitW = Normalize(State->Direction);
    real32 KDotW = Dot(UnitK, UnitW);
    real32 KDotW2 = Square(Square(KDotW));

    real32 WLen = Length(State->Direction);
    real32 L = Square(WLen) / g_G;
    real32 L2 = Square(L);

    real32 Damping = 1e-3f;
    real32 DampL2 = L2 * Square(Damping);

    return State->Amplitude * (expf(-1.f / (KLen2 * L2)) / KLen4) * KDotW2 * expf(-KLen2 * DampL2);
}

real32 ComputeDispersion(real32 Width, int n_prime, int m_prime)
{
    real32 W0 = 2.f * M_PI / 200.f;
    real32 Kx = M_PI * (2 * n_prime - water::system::WaterN) / Width;
    real32 Kz = M_PI * (2 * m_prime - water::system::WaterN) / Width;
    return floorf(sqrtf(g_G * sqrtf(Square(Kx) + Square(Kz))) / W0) * W0;
}

complex ComputeHTilde0(water::beaufort_state *State, int n_prime, int m_prime)
{
    complex R = GaussianRandomVariable();
    return R * sqrtf(Phillips(State, n_prime, m_prime) / 2.0f);
}

complex ComputeHTilde(water::beaufort_state *StateA, water::beaufort_state *StateB, real32 WaterInterp, 
        real32 T, int n_prime, int m_prime)
{
    int NPlus1 = water::system::WaterN+1;
    int Idx = m_prime * NPlus1 + n_prime;

    vec3f *HTilde0A = (vec3f*)StateA->HTilde0;
    vec3f *HTilde0B = (vec3f*)StateB->HTilde0;
    vec3f *HTilde0mkA = (vec3f*)StateA->HTilde0mk;
    vec3f *HTilde0mkB = (vec3f*)StateB->HTilde0mk;

    vec3f dHT0 = Lerp(HTilde0A[Idx], HTilde0B[Idx], WaterInterp);
    vec3f dHT0mk = Lerp(HTilde0mkA[Idx], HTilde0mkB[Idx], WaterInterp);
    real32 dWidth = Lerp((real32)StateA->Width, (real32)StateB->Width, WaterInterp);
    
    complex H0(dHT0.x, dHT0.y);
    complex H0mk(dHT0mk.x, dHT0mk.y);

    real32 OmegaT = ComputeDispersion(dWidth, n_prime, m_prime) * T;
    real32 CosOT = cosf(OmegaT);
    real32 SinOT = sinf(OmegaT);

    complex C0(CosOT, SinOT);
    complex C1(CosOT, -SinOT);

    return H0 * C0 + H0mk * C1;
}

uint32 FFTReverse(uint32 i, int Log2N)
{
    uint32 Res = 0;
    for(int j = 0; j < Log2N; ++j)
    {
        Res = (Res << 1) + (i & 1);
        i >>= 1;
    }
    return Res;
}

complex FFTW(uint32 X, uint32 N)
{
    float V = M_TWO_PI * X / N;
    return complex(cosf(V), sinf(V));
}

void FFTEvaluate(water::system *WS, complex *Input, complex *Output, int Stride, int Offset, int N)
{
    for(int i = 0; i < N; ++i)
        WS->FFTC[WS->Switch][i] = Input[WS->Reversed[i] * Stride + Offset];

    int Loops = N >> 1;
    int Size = 2;
    int SizeOver2 = 1;
    int W_ = 0;
    for(int j = 1; j <= WS->Log2N; ++j)
    {
        WS->Switch ^= 1;
        for(int i = 0; i < Loops; ++i)
        {
            complex *FFTCDst = WS->FFTC[WS->Switch];
            complex *FFTCSrc = WS->FFTC[WS->Switch^1];
            for(int k = 0; k < SizeOver2; ++k)
            {
                FFTCDst[Size * i + k] = FFTCSrc[Size * i + k] +
                                        FFTCSrc[Size * i + SizeOver2 + k] * WS->FFTW[W_][k];
            }
            for(int k = SizeOver2; k < Size; ++k)
            {
                FFTCDst[Size * i + k] = FFTCSrc[Size * i - SizeOver2 + k] -
                                        FFTCSrc[Size * i + k] * WS->FFTW[W_][k - SizeOver2];
            }
        }
        Loops >>= 1;
        Size <<= 1;
        SizeOver2 <<= 1;
        W_++;
    }

    for(int i = 0; i < N; ++i)
        Output[i * Stride + Offset] = WS->FFTC[WS->Switch][i];
}

void UpdateWaterMesh(water::system *WaterSystem)
{
    glBindVertexArray(WaterSystem->VAO);
    size_t VertSize = WaterSystem->VertexCount * sizeof(real32);
    rf::UpdateVBO(WaterSystem->VBO[1], 0, VertSize, WaterSystem->VertexData);
    rf::UpdateVBO(WaterSystem->VBO[1], VertSize, VertSize, WaterSystem->VertexData + WaterSystem->VertexCount);

    glBindVertexArray(0);
}

void WaterBeaufortStateInitialize(water::system *WaterSystem, uint32 State)
{
    water::beaufort_state *WaterState = &WaterSystem->States[State];
    int N = water::system::WaterN;
    int NPlus1 = N+1;

    WaterState->Width = (int)BeaufortParams[State][0] * water::system::WaterN;
    WaterState->Direction = vec2f(BeaufortParams[State][1] * water::system::WaterN, 0.0f);
    WaterState->Amplitude = 0.00000025f * BeaufortParams[State][2] * water::system::WaterN;

    size_t BaseOffset = 2 * WaterSystem->VertexCount;
    WaterState->OrigPositions = WaterSystem->VertexData + BaseOffset + (State * 3 + 0) * WaterSystem->VertexCount;
    WaterState->HTilde0 = WaterSystem->VertexData + BaseOffset + (State * 3 + 1) * WaterSystem->VertexCount;
    WaterState->HTilde0mk = WaterSystem->VertexData + BaseOffset + (State * 3 + 2) * WaterSystem->VertexCount;

    vec3f *OrigPositions = (vec3f*)WaterState->OrigPositions;
    vec3f *HTilde0 = (vec3f*)WaterState->HTilde0;
    vec3f *HTilde0mk = (vec3f*)WaterState->HTilde0mk;

    for(int m_prime = 0; m_prime < NPlus1; m_prime++)
    {
        for(int n_prime = 0; n_prime < NPlus1; n_prime++)
        {
            int Idx = m_prime * NPlus1 + n_prime;
            complex H0 = ComputeHTilde0(WaterState, n_prime, m_prime);
            complex H0mk = Conjugate(ComputeHTilde0(WaterState, -n_prime, -m_prime));

            OrigPositions[Idx].x = (n_prime - N / 2.0f) * WaterState->Width / N;
            OrigPositions[Idx].y = 0.f;
            OrigPositions[Idx].z = (m_prime - N / 2.0f) * WaterState->Width / N;

            HTilde0[Idx].x = H0.r;
            HTilde0[Idx].y = H0.i;
            HTilde0mk[Idx].x = H0mk.r;
            HTilde0mk[Idx].y = H0mk.i;
        }
    }
}

namespace water {
water::system *WaterSystem = NULL;
rf::mesh ScreenQuad = {};

void Init(game::state * /*State*/, rf::context *Context, uint32 /*BeaufortState*/)
{
    ScreenQuad = rf::Make2DQuad(Context, vec2i(-1,1), vec2i(1, -1), 5);
    WaterSystem = rf::PoolAlloc<water::system>(Context->SessionPool, 1);
#if 0
    int N = water::system::WaterN;
    int NPlus1 = N+1;


    size_t WaterStateAttribs = 3 * sizeof(vec3f); // hTilde0, hTilde0mk, OrigPos
    size_t WaterAttribs = 2 * sizeof(vec3f); // Pos, Norm
    size_t WaterVertexDataSize = Square(NPlus1) * (WaterAttribs + water::system::BeaufortStateCount * WaterStateAttribs);
    size_t WaterVertexCount = 3 * Square(NPlus1); // 3 floats per attrib
    real32 *WaterVertexData = (real32*)PushArenaData(Context->SessionArena, WaterVertexDataSize);

    size_t WaterIndexDataSize = Square(N) * 6 * sizeof(uint32);
    uint32 *WaterIndexData = (uint32*)PushArenaData(Context->SessionArena, WaterIndexDataSize);


    WaterSystem->VertexDataSize = WaterVertexDataSize;
    WaterSystem->VertexCount = WaterVertexCount;
    WaterSystem->VertexData = WaterVertexData;
    WaterSystem->IndexDataSize = WaterIndexDataSize;
    WaterSystem->IndexData = WaterIndexData;
    WaterSystem->Positions = WaterSystem->VertexData;
    WaterSystem->Normals = WaterSystem->VertexData + WaterVertexCount;

    WaterSystem->hTilde = (complex*)PushArenaData(Context->SessionArena, N * N * sizeof(complex));
    WaterSystem->hTildeSlopeX = (complex*)PushArenaData(Context->SessionArena, N * N * sizeof(complex));
    WaterSystem->hTildeSlopeZ = (complex*)PushArenaData(Context->SessionArena, N * N * sizeof(complex));
    WaterSystem->hTildeDX = (complex*)PushArenaData(Context->SessionArena, N * N * sizeof(complex));
    WaterSystem->hTildeDZ = (complex*)PushArenaData(Context->SessionArena, N * N * sizeof(complex));

    WaterSystem->Switch = 0;
    WaterSystem->Log2N = log(N) / log(2);
    WaterSystem->FFTC[0] = (complex*)PushArenaData(Context->SessionArena, N * sizeof(complex));
    WaterSystem->FFTC[1] = (complex*)PushArenaData(Context->SessionArena, N * sizeof(complex));
    WaterSystem->Reversed = (uint32*)PushArenaData(Context->SessionArena, N * sizeof(uint32));
    for(int i = 0; i < N; ++i)
    {
        WaterSystem->Reversed[i] = FFTReverse(i, WaterSystem->Log2N);
    }
    WaterSystem->FFTW = (complex**)PushArenaData(Context->SessionArena, WaterSystem->Log2N * sizeof(complex*));
    int Pow2 = 1;
    for(int j = 0; j < WaterSystem->Log2N; ++j)
    {
        WaterSystem->FFTW[j] = (complex*)PushArenaData(Context->SessionArena, Pow2 * sizeof(complex));
        for(int i = 0; i < Pow2; ++i)
            WaterSystem->FFTW[j][i] = FFTW(i, 2 * Pow2);
        Pow2 *=2;
    }
    
    for(uint32 i = 0; i < water::system::BeaufortStateCount; ++i)
    {
        WaterBeaufortStateInitialize(WaterSystem, i);
    }

    vec3f *Positions = (vec3f*)WaterSystem->Positions;
    vec3f *Normals = (vec3f*)WaterSystem->Normals;
    uint32 *Indices = (uint32*)WaterSystem->IndexData;

    vec3f *OrigPositions = (vec3f*)WaterSystem->States[BeaufortState].OrigPositions;
    for(int m_prime = 0; m_prime < NPlus1; m_prime++)
    {
        for(int n_prime = 0; n_prime < NPlus1; n_prime++)
        {
            int Idx = m_prime * NPlus1 + n_prime;
            Positions[Idx].x = OrigPositions[Idx].x;
            Positions[Idx].y = OrigPositions[Idx].y;
            Positions[Idx].z = OrigPositions[Idx].z;

            Normals[Idx] = vec3f(0, 1, 0);
        }
    }

    uint32 IndexCount = 0;
    for(int m_prime = 0; m_prime < N; m_prime++)
    {
        for(int n_prime = 0; n_prime < N; n_prime++)
        {
            int Idx = m_prime * NPlus1 + n_prime;
            
            Indices[IndexCount++] = Idx;
            Indices[IndexCount++] = Idx + NPlus1;
            Indices[IndexCount++] = Idx + NPlus1 + 1;
            Indices[IndexCount++] = Idx;
            Indices[IndexCount++] = Idx + NPlus1 + 1;
            Indices[IndexCount++] = Idx + 1;
        }
    }
    WaterSystem->IndexCount = IndexCount;
    WaterSystem->VAO = rf::MakeVertexArrayObject();
    WaterSystem->VBO[0] = rf::AddIBO(GL_STATIC_DRAW, WaterSystem->IndexCount * sizeof(uint32), WaterSystem->IndexData);
    WaterSystem->VBO[1] = rf::AddEmptyVBO(WaterSystem->VertexDataSize, GL_STATIC_DRAW);
    size_t VertSize = WaterSystem->VertexCount * sizeof(real32);
    rf::FillVBO(0, 3, GL_FLOAT, 0, VertSize, WaterSystem->VertexData);
    rf::FillVBO(1, 3, GL_FLOAT, VertSize, VertSize, WaterSystem->VertexData + WaterSystem->VertexCount);
    rf::FillVBO(2, 3, GL_FLOAT, 2*VertSize, VertSize, WaterSystem->VertexData + 2 * WaterSystem->VertexCount);
    glBindVertexArray(0);
#endif
}

void Update(game::state * /*State*/, rf::input * /*Input*/)
{
#if 0
    beaufort_state *WStateA = &WaterSystem->States[State->WaterState];
    beaufort_state *WStateB = &WaterSystem->States[State->WaterState + 1];

    State->WaterCounter += Input->dTime;

    vec3f *WaterPositions = (vec3f*)WaterSystem->Positions;
    vec3f *WaterNormals = (vec3f*)WaterSystem->Normals;

    vec3f *WaterOrigPositionsA = (vec3f*)WStateA->OrigPositions;
    vec3f *WaterOrigPositionsB = (vec3f*)WStateB->OrigPositions;

    real32 dT = (real32)State->WaterCounter;

    int N = water::system::WaterN;
    int NPlus1 = N+1;

    float Lambda = -1.0f;

    complex *hT = (complex*)WaterSystem->hTilde;
    complex *hTSX = (complex*)WaterSystem->hTildeSlopeX;
    complex *hTSZ = (complex*)WaterSystem->hTildeSlopeZ;
    complex *hTDX = (complex*)WaterSystem->hTildeDX;
    complex *hTDZ = (complex*)WaterSystem->hTildeDZ;

    real32 dWidth = Mix((real32)WStateA->Width, (real32)WStateB->Width, State->WaterStateInterp);

    // Prepare
    for(int m_prime = 0; m_prime < N; ++m_prime)
    {
        real32 Kz = M_PI * (2.f * m_prime - N) / dWidth;
        for(int n_prime = 0; n_prime < N; ++n_prime)
        {
            real32 Kx = M_PI * (2.f * n_prime - N) / dWidth;
            real32 Len = sqrtf(Square(Kx) + Square(Kz));
            int Idx = m_prime * N + n_prime;

            hT[Idx] = ComputeHTilde(WStateA, WStateB, State->WaterStateInterp, dT, n_prime, m_prime);
            hTSX[Idx] = hT[Idx] * complex(0, Kx);
            hTSZ[Idx] = hT[Idx] * complex(0, Kz);
            if(Len < 1e-6f)
            {
                hTDX[Idx] = complex(0, 0);
                hTDZ[Idx] = complex(0, 0);
            } else {
                hTDX[Idx] = hT[Idx] * complex(0, -Kx/Len);
                hTDZ[Idx] = hT[Idx] * complex(0, -Kz/Len);
            }
        }
    }

    // Evaluate
    for(int m_prime = 0; m_prime < N; ++m_prime)
    {
        FFTEvaluate(WaterSystem, hT, hT, 1, m_prime * N, N);
        FFTEvaluate(WaterSystem, hTSX, hTSX, 1, m_prime * N, N);
        FFTEvaluate(WaterSystem, hTSZ, hTSZ, 1, m_prime * N, N);
        FFTEvaluate(WaterSystem, hTDX, hTDX, 1, m_prime * N, N);
        FFTEvaluate(WaterSystem, hTDZ, hTDZ, 1, m_prime * N, N);
    }

    for(int n_prime = 0; n_prime < N; ++n_prime)
    {
        FFTEvaluate(WaterSystem, hT, hT, N, n_prime, N);
        FFTEvaluate(WaterSystem, hTSX, hTSX, N, n_prime, N);
        FFTEvaluate(WaterSystem, hTSZ, hTSZ, N, n_prime, N);
        FFTEvaluate(WaterSystem, hTDX, hTDX, N, n_prime, N);
        FFTEvaluate(WaterSystem, hTDZ, hTDZ, N, n_prime, N);
    }

    // Fill results
    float Signs[] = { 1.f, -1.f };
    for(int m_prime = 0; m_prime < N; ++m_prime)
    {
        for(int n_prime = 0; n_prime < N; ++n_prime)
        {
            int Idx = m_prime * N + n_prime;        // for htilde
            int Idx1 = m_prime * NPlus1 + n_prime;  // for vertices

            int Sign = Signs[(n_prime + m_prime) & 1];

            hT[Idx] = hT[Idx] * Sign;
            WaterPositions[Idx1].y = hT[Idx].r;

            hTDX[Idx] = hTDX[Idx] * Sign;
            hTDZ[Idx] = hTDZ[Idx] * Sign;
            {
                vec3f OP = Mix(WaterOrigPositionsA[Idx1], WaterOrigPositionsB[Idx1], State->WaterStateInterp);
                WaterPositions[Idx1].x = OP.x + Lambda * hTDX[Idx].r;
                WaterPositions[Idx1].z = OP.z + Lambda * hTDZ[Idx].r;
            }

            hTSX[Idx] = hTSX[Idx] * Sign;
            hTSZ[Idx] = hTSZ[Idx] * Sign;
            vec3f Normal = Normalize(vec3f(-hTSX[Idx].r, 1, -hTSZ[Idx].r));

            WaterNormals[Idx1] = Normal;

            if(n_prime == 0 && m_prime == 0)
            {
                vec3f OP = Mix(WaterOrigPositionsA[Idx1 + N + NPlus1 * N], WaterOrigPositionsB[Idx1 + N + NPlus1 * N], State->WaterStateInterp);
                WaterPositions[Idx1 + N + NPlus1 * N].x = OP.x + Lambda * hTDX[Idx].r;
                WaterPositions[Idx1 + N + NPlus1 * N].y = hT[Idx].r;
                WaterPositions[Idx1 + N + NPlus1 * N].z = OP.z + Lambda * hTDZ[Idx].r;

                WaterNormals[Idx1 + N + NPlus1 * N] = Normal;
            }
            if(n_prime == 0)
            {
                vec3f OP = Mix(WaterOrigPositionsA[Idx1 + N], WaterOrigPositionsB[Idx1 + N], State->WaterStateInterp);
                WaterPositions[Idx1 + N].x = OP.x + Lambda * hTDX[Idx].r;
                WaterPositions[Idx1 + N].y = hT[Idx].r;
                WaterPositions[Idx1 + N].z = OP.z + Lambda * hTDZ[Idx].r;

                WaterNormals[Idx1 + N] = Normal;
            }
            if(m_prime == 0)
            {
                vec3f OP = Mix(WaterOrigPositionsA[Idx1 + NPlus1 * N], WaterOrigPositionsB[Idx1 + NPlus1 * N], State->WaterStateInterp);
                WaterPositions[Idx1 + NPlus1 * N].x = OP.x + Lambda * hTDX[Idx].r;
                WaterPositions[Idx1 + NPlus1 * N].y = hT[Idx].r;
                WaterPositions[Idx1 + NPlus1 * N].z = OP.z + Lambda * hTDZ[Idx].r;

                WaterNormals[Idx1 + NPlus1 * N] = Normal;
            }
        }
    }
    UpdateWaterMesh(WaterSystem);
#endif
}

real32 IntersectPlane(vec3f const &N, vec3f const &P0, vec3f const &RayOrg, vec3f const &RayDir)
{
    real32 Denom = Dot(N, RayDir);
    if(std::fabs(Denom) > 1e-5)
    {
        vec3f P0Org = P0 - RayOrg;
        real32 T = Dot(P0Org, N) / Denom;
        if(T >= 0.f)
            return T;
        return -1.f;
    }
    return -1.f;
}

static const float S_Upper = 5.f; // TODO - this is the upper level of displacement, should be parameterized
static const float M2_FixedLength = 10.f; // Fixed length along the Fwd vector for method 2 projector direction

void GetProjectorPositionAndDirection(const camera &Camera, 
                                      vec3f &ProjectorPosition, vec3f &ProjectorTarget)
{
    ProjectorPosition = Camera.Position + Camera.PositionDecimal;
    //ProjectorPosition.y = Max(ProjectorPosition.y, S_Upper); // limit Projector height above upper displacement 

    // Get projector orientation 
    vec3f D = Camera.Forward;
    vec3f N = vec3f(0,1,0); // plane normal
    real32 NdotD = Dot(D, N);
    if(NdotD > 0.f)
    { // Mirror direction if not towards plane
        D.y = -D.y;
        NdotD = -NdotD;
    }

    // method 1 : to the intersection of the camera direction and the sea plane
    real32 T = IntersectPlane(N, vec3f(0,0,0), ProjectorPosition, D);
    Assert(T >= 0.f);
    vec3f M1 = ProjectorPosition + D * T;

    // method 2 : to the 2d plane projection of a fixed length direction along the fwd vector
    vec3f M2 = ProjectorPosition + Camera.Forward * M2_FixedLength;
    M2.y = 0.f;

    // The result is a linear mix between the two methods depending on the angle of the cam
    // view direction and the sea normal
    ProjectorTarget = Lerp(M2, M1, NdotD);
}

void Render(game::state *State, uint32 /*Envmap*/, uint32 /*GGXLUT*/)
{
    glDisable(GL_CULL_FACE);
    glUseProgram(WaterSystem->ProgramWater);
    {
        uint32 Loc = glGetUniformLocation(WaterSystem->ProgramWater, "ViewMatrix");
        rf::SendMat4(Loc, State->Camera.ViewMatrix);
        rf::CheckGLError("ViewMatrix");
    }

#if 0
    camera WPC = State->Camera;
    WPC.Theta = Min(WPC.Theta, (real32(M_PI) * 0.5) - 1e-5f);
    WPC.Theta = WPC.Theta - sin(WPC.Theta);
    WPC.Forward = SphericalToCartesian(WPC.Theta, WPC.Phi);
    WPC.Right = Normalize(Cross(WPC.Forward, vec3f(0, 1, 0)));
    WPC.Up = Normalize(Cross(WPC.Right, WPC.Forward));
    WPC.Target = WPC.Position + WPC.Forward;
    WPC.ViewMatrix = mat4f::LookAt(WPC.Position, WPC.Target, WPC.Up);
    vec3f Pos = State->Camera.Position;
    vec3f Target = Pos + vec3f(0,-1,0);
    vec3f Fwd = Normalize(Target - Fwd);
    vec3f Right = Normalize(Cross(Fwd, vec3f(0, 1, 0)));
    vec3f Up = Normalize(Cross(Right, Fwd));
#endif

    vec3f ProjPos, ProjTarget;
    GetProjectorPositionAndDirection(State->Camera, ProjPos, ProjTarget);
    vec3f ProjFwd = Normalize(ProjTarget - ProjPos);
    vec3f ProjRight = Normalize(Cross(ProjFwd, vec3f(0,1,0)));
    vec3f ProjUp = Normalize(Cross(ProjRight, ProjFwd));
    mat4f ProjectorMatrix = mat4f::LookAt(ProjPos, ProjTarget, ProjUp);

    rf::SendFloat(glGetUniformLocation(WaterSystem->ProgramWater, "Time"), (real32)State->EngineTime);
    rf::SendVec3(glGetUniformLocation(WaterSystem->ProgramWater, "ProjectorPosition"), ProjPos);
    rf::SendMat4(glGetUniformLocation(WaterSystem->ProgramWater, "WaterProjMatrix"), ProjectorMatrix);
    glBindVertexArray(ScreenQuad.VAO);
    RenderMesh(&ScreenQuad);
    glEnable(GL_CULL_FACE);
#if 0
    glUseProgram(WaterSystem->ProgramWater);
    glDisable(GL_CULL_FACE);
    {
        uint32 Loc = glGetUniformLocation(WaterSystem->ProgramWater, "ViewMatrix");
        rf::SendMat4(Loc, State->Camera.ViewMatrix);
        rf::CheckGLError("ViewMatrix");
    }
    uint32 Loc = glGetUniformLocation(WaterSystem->ProgramWater, "LightColor");
    rf::SendVec4(Loc, State->LightColor);
    Loc = glGetUniformLocation(WaterSystem->ProgramWater, "CameraPos");
    rf::SendVec3(Loc, State->Camera.Position);
    Loc = glGetUniformLocation(WaterSystem->ProgramWater, "SunDirection");
    rf::SendVec3(Loc, State->SunDirection);
    Loc = glGetUniformLocation(WaterSystem->ProgramWater, "Time");
    rf::SendFloat(Loc, State->EngineTime);


    Loc = glGetUniformLocation(WaterSystem->ProgramWater, "ModelMatrix");
    mat4f ModelMatrix;
    glBindVertexArray(WaterSystem->VAO);

    beaufort_state *WaterStateA = &WaterSystem->States[State->WaterState];
    beaufort_state *WaterStateB = &WaterSystem->States[State->WaterState + 1];
    real32 dWidth = Mix((real32)WaterStateA->Width, (real32)WaterStateB->Width, State->WaterStateInterp);
    //dWidth *= (State->WaterState+1) * 1.5;

    real32 Interp = (State->WaterState+1) + State->WaterStateInterp;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GGXLUT);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Envmap);


    int Repeat = 1;
    int Middle = (Repeat-1)/2;
    for(int j = 0; j < Repeat; ++j)
    {
        for(int i = 0; i < Repeat; ++i)
        {
            // MIDDLE
            real32 PositionScale = dWidth * (Interp);
            vec3f Position(PositionScale * (Middle-i), 0.f, PositionScale * (Middle-j));
            vec3f Scale(Interp, Interp, Interp);
            vec3f Rotation(0, State->WaterDirection, 0);
            mat4f RotationMatrix;
            RotationMatrix.FromAxisAngle(Rotation);
            ModelMatrix.FromTRS(Position, vec3f(0), Scale);
            ModelMatrix = RotationMatrix * ModelMatrix;

            rf::SendMat4(Loc, ModelMatrix);
            glDrawElements(GL_TRIANGLES, WaterSystem->IndexCount, GL_UNSIGNED_INT, 0);
        }
    }
    glEnable(GL_CULL_FACE);
#endif
}

void ReloadShaders(rf::context *Context)
{
    path VSPath, FSPath;

    rf::ConcatStrings(VSPath, rf::ctx::GetExePath(Context), "data/shaders/water_vert.glsl");
    rf::ConcatStrings(FSPath, rf::ctx::GetExePath(Context), "data/shaders/water_frag.glsl");
    WaterSystem->ProgramWater = rf::BuildShader(Context, VSPath, FSPath);
    glUseProgram(WaterSystem->ProgramWater);
    //rf::SendInt(glGetUniformLocation(WaterSystem->ProgramWater, "GGXLUT"), 0);
    //rf::SendInt(glGetUniformLocation(WaterSystem->ProgramWater, "Skybox"), 1);
    rf::CheckGLError("Water Shader");

    rf::ctx::RegisterShader3D(Context, WaterSystem->ProgramWater);

    /*
    MakeRelativePath(RH, VSPath, "data/shaders/waterproj_vert.glsl");
    MakeRelativePath(RH, VSPath, "data/shaders/waterproj_frag.glsl");
    ProjWaterProgram = BuildShader(Memory, VSPath, FSPath);
    glUseProgram(ProjWaterProgram);
    CheckGLError("Proj Water Shader");

    context::RegisterShader3D(Context, ProjWaterProgram);
    */
}

}
