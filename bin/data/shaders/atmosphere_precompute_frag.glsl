#version 400

// TODO - give them as uniform from program constants, or create the shader from the code with this hardcoded
#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

#define IRRADIANCE_TEXTURE_WIDTH 64
#define IRRADIANCE_TEXTURE_HEIGHT 16

#define SCATTERING_TEXTURE_R_SIZE 32
#define SCATTERING_TEXTURE_MU_SIZE 128
#define SCATTERING_TEXTURE_MU_S_SIZE 32
#define SCATTERING_TEXTURE_NU_SIZE 8

#define SCATTERING_TEXTURE_WIDTH (SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE)
#define SCATTERING_TEXTURE_HEIGHT SCATTERING_TEXTURE_MU_SIZE
#define SCATTERING_TEXTURE_DEPTH SCATTERING_TEXTURE_R_SIZE

// TODO - put the structs and common functions in a shared header
struct density_profile_layer
{
    float Width;
    float ExpTerm;
    float ExpScale;
    float LinearTerm;
    float ConstantTerm;
};

struct density_profile
{
    density_profile_layer Layers[2];
};

struct atmosphere_parameters
{
    float           TopRadius;
    float           BottomRadius;
    vec3            RayleighScattering;
    density_profile RayleighDensity;
    vec3            MieExtinction;
    vec3            MieScattering;
    density_profile MieDensity;
    vec3            AbsorptionExtinction;
    density_profile AbsorptionDensity;
    vec3            GroundAlbedo;
    vec3            SolarIrradiance;
    float           SunAngularRadius;
    float           MiePhaseG;
    float           MinMuS;
};

uniform atmosphere_parameters Atmosphere;
uniform int ProgramUnit;
uniform sampler2D Tex0;
uniform int ScatteringLayer;

out vec4 out0;
out vec4 out1;
out vec4 out2;

float ClampDistance(float D)
{
    return max(D, 0.0);
}

float ClampCosine(float C)
{
    return clamp(C, -1.0, 1.0);
}

float ClampRadius(float R)
{
   return clamp(R, Atmosphere.BottomRadius, Atmosphere.TopRadius); 
}

float SafeSqrt(float V)
{
    return sqrt(max(V, 0));
}

float DistanceToBottomBoundary(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + Atmosphere.BottomRadius * Atmosphere.BottomRadius;
    return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

float DistanceToTopBoundary(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + Atmosphere.TopRadius * Atmosphere.TopRadius;
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

float DistanceToNearestAtmosphereBoundary(float r, float mu, bool IntersectsGround)
{
    if(IntersectsGround)
    {
        return DistanceToBottomBoundary(r, mu);
    }
    else
    {
        return DistanceToTopBoundary(r, mu);
    }
}

// Mappings from x in [0,1] to texcoord u in [0.5/n, 1 - 0.5/n]
float GetTextureCoordFromUnitRange(float x, int TexSize)
{
    return 0.5 / float(TexSize) + x * (1.0 - 1.0 / float(TexSize));
}

float GetUnitRangeFromTextureCoord(float u, int TexSize)
{
    return (u - 0.5 / float(TexSize)) / (1.0 - 1.0 / float(TexSize));
}

vec2 GetTransmittanceTextureUVFromRMu(float r, float mu)
{
    float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float rho = SafeSqrt(r * r - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float d = DistanceToTopBoundary(r, mu);
    float d_min = Atmosphere.TopRadius - r;
    float d_max = rho + H;
    float x_mu = (d - d_min) / (d_max - d_min);
    float x_r = rho / H;
    return vec2(GetTextureCoordFromUnitRange(x_mu, TRANSMITTANCE_TEXTURE_WIDTH), GetTextureCoordFromUnitRange(x_r, TRANSMITTANCE_TEXTURE_HEIGHT));
}

void GetRMuFromTransmittanceTextureUV(vec2 uv, out float r, out float mu)
{
    float x_mu = GetUnitRangeFromTextureCoord(uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
    float x_r = GetUnitRangeFromTextureCoord(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);
    float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float rho = SafeSqrt(x_r * x_r - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    r = sqrt(rho * rho + Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float d_min = Atmosphere.TopRadius - r;
    float d_max = rho + H;
    float d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * r * d);
    mu = ClampCosine(mu);
}

vec2 GetIrradianceTextureUVFromRMuS(float r, float mu_s)
{
    float x_r = (r - Atmosphere.BottomRadius) / (Atmosphere.TopRadius - Atmosphere.BottomRadius);
    float x_mu_s = mu_s * 0.5 + 0.5;
    return vec2(GetTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH), GetTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
}

void GetRMuSFromIrradianceTextureUV(vec2 uv, out float r, out float mu_s)
{
    float x_mu_s = GetUnitRangeFromTextureCoord(uv.x, IRRADIANCE_TEXTURE_WIDTH);
    float x_r = GetUnitRangeFromTextureCoord(uv.y, IRRADIANCE_TEXTURE_HEIGHT);
    r = Atmosphere.BottomRadius + x_r * (Atmosphere.TopRadius - Atmosphere.BottomRadius);
    mu_s = ClampCosine(2.0 * x_mu_s - 1.0);
}

vec4 GetScatteringTextureUVWZFromRMuMuSNu(float r, float mu, float mu_s, float nu, bool IntersectsGround)
{
    float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float rho = SafeSqrt(r * r - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float u_r = GetTextureCoordFromUnitRange(rho / H, SCATTERING_TEXTURE_R_SIZE);

    float r_mu = r * mu;
    float discriminant = r_mu * r_mu - r * r + Atmosphere.BottomRadius * Atmosphere.BottomRadius; // from RayIntersectsGround
    float u_mu;
    if(IntersectsGround)
    {
        float d = -r_mu - SafeSqrt(discriminant);
        float d_min = r - Atmosphere.BottomRadius;
        float d_max = rho;
        u_mu = 0.5 - 0.5 * GetTextureCoordFromUnitRange(d_max == d_min ? 0.0 : (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
    }
    else
    {
        float d = -r_mu + SafeSqrt(discriminant + H * H);
        float d_min = Atmosphere.TopRadius - r;
        float d_max = rho + H;
        u_mu = 0.5 + 0.5 * GetTextureCoordFromUnitRange((d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
    }

    float d = DistanceToTopBoundary(Atmosphere.BottomRadius, mu_s);
    float d_min = Atmosphere.TopRadius - Atmosphere.BottomRadius;
    float d_max = H;
    float a = (d - d_min) / (d_max - d_min);
    float A = -2.0 * Atmosphere.MinMuS * Atmosphere.BottomRadius / (d_max - d_min);
    float u_mu_s = GetTextureCoordFromUnitRange(max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);
    float u_nu = (nu + 1.0) / 2.0;
    return vec4(u_nu, u_mu_s, u_mu, u_r);
}

void GetRMuMuSNuFromScatteringTextureUVWZ(in vec4 uvwz, out float r, out float mu, out float mu_s, out float nu, out bool IntersectsGround)
{
    float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float rho = H * GetUnitRangeFromTextureCoord(uvwz.w, SCATTERING_TEXTURE_R_SIZE);
    r = sqrt(rho * rho + Atmosphere.BottomRadius * Atmosphere.BottomRadius);

    if(uvwz.z < 0.5)
    {
        float d_min = r - Atmosphere.BottomRadius;
        float d_max = rho;
        float d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(1.0 - 2.0 * uvwz.z, SCATTERING_TEXTURE_MU_SIZE / 2);
        mu = d == 0.0 ? -1.0 : ClampCosine(-(rho * rho + d * d) / (2.0 * r * d));
        IntersectsGround = true;
    }
    else
    {
        float d_min = Atmosphere.TopRadius - r;
        float d_max = rho + H;
        float d = d_min + (d_max - d_min) * GetUnitRangeFromTextureCoord(2.0 * uvwz.z - 1.0, SCATTERING_TEXTURE_MU_SIZE / 2);
        mu = d == 0.0 ? 1.0 : ClampCosine((H * H - rho * rho - d * d) / (2.0 * r * d));
        IntersectsGround = false;
    }

    float x_mu_s = GetUnitRangeFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
    float d_min = Atmosphere.TopRadius - Atmosphere.BottomRadius;
    float d_max = H;
    float A = -2.0 * Atmosphere.MinMuS * Atmosphere.BottomRadius / (d_max - d_min);
    float a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
    float d = d_min + min(a, A) * (d_max - d_min);
    mu_s = d == 0.0 ? 1.0 : ClampCosine((H * H - d * d) / (2.0 * Atmosphere.BottomRadius * d));
    nu = ClampCosine(uvwz.x * 2.0 - 1.0);
}

void GetRMuMuSNuFromScatteringTextureFragCoord(in vec3 FragCoord, out float r, out float mu, out float mu_s, out float nu, out bool IntersectsGround)
{
    vec4 SCATTERING_TEXTURE_SIZE = vec4(SCATTERING_TEXTURE_NU_SIZE - 1, SCATTERING_TEXTURE_MU_S_SIZE, SCATTERING_TEXTURE_MU_SIZE, SCATTERING_TEXTURE_R_SIZE);
    float fragcoord_nu = floor(FragCoord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));
    float fragcoord_mu_s = mod(FragCoord.x, float(SCATTERING_TEXTURE_MU_S_SIZE));
    vec4 uvwz = vec4(fragcoord_nu, fragcoord_mu_s, FragCoord.y, FragCoord.z) / SCATTERING_TEXTURE_SIZE;
    GetRMuMuSNuFromScatteringTextureUVWZ(uvwz, r, mu, mu_s, nu, IntersectsGround);
    nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)), mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}

float GetLayerDensity(in density_profile_layer layer, float altitude)
{
    float density = layer.ExpTerm * exp(layer.ExpScale * altitude) + layer.LinearTerm * altitude + layer.ConstantTerm;
    return clamp(density, 0.0, 1.0);
}

float GetProfileDensity(in density_profile profile, float altitude)
{
    return altitude < profile.Layers[0].Width ? GetLayerDensity(profile.Layers[0], altitude)
                                              : GetLayerDensity(profile.Layers[1], altitude);
}

float ComputeOpticalLengthToTopAtmosphereBoundary(in density_profile profile, float r, float mu)
{
    int SAMPLE_COUNT = 500;
    float dx = DistanceToTopBoundary(r, mu) / float(SAMPLE_COUNT);
    float result = 0.0;
    for(int i = 0; i < SAMPLE_COUNT; ++i)
    {
        float d_i = float(i) * dx;
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        float y_i = GetProfileDensity(profile, r_i - Atmosphere.BottomRadius);
        float w_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
        result += y_i * w_i * dx;
    }
    return result;
}

vec3 ComputeTransmittanceToTopAtmosphereBoundary(float r, float mu)
{
    return exp(-(Atmosphere.RayleighScattering * ComputeOpticalLengthToTopAtmosphereBoundary(Atmosphere.RayleighDensity, r, mu)
                 + Atmosphere.MieExtinction * ComputeOpticalLengthToTopAtmosphereBoundary(Atmosphere.MieDensity, r, mu)
                 + Atmosphere.AbsorptionExtinction * ComputeOpticalLengthToTopAtmosphereBoundary(Atmosphere.AbsorptionDensity, r, mu)
                 ));
}
vec3 ComputeTransmittanceToTopAtmosphereBoundaryTexture(vec2 FragCoord)
{
    vec2 TexSize = vec2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    float r, mu;
    GetRMuFromTransmittanceTextureUV(FragCoord / TexSize, r, mu);
    return ComputeTransmittanceToTopAtmosphereBoundary(r, mu);
}

vec3 GetTransmittanceToTopAtmosphereBoundary(in sampler2D TransmittanceTexture, float r, float mu)
{
    vec2 uv = GetTransmittanceTextureUVFromRMu(r, mu);
    return texture(TransmittanceTexture, uv).xyz;
}

vec3 GetTransmittance(in sampler2D TransmittanceTexture, float r, float mu, float d, bool IntersectsGround)
{
    float r_d = ClampRadius(sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu + d) / r_d);

    if(IntersectsGround)
    {
        return min(GetTransmittanceToTopAtmosphereBoundary(TransmittanceTexture, r_d, -mu_d) /
                   GetTransmittanceToTopAtmosphereBoundary(TransmittanceTexture, r, -mu), 
                   vec3(1));
    }
    else
    {
        return min(GetTransmittanceToTopAtmosphereBoundary(TransmittanceTexture, r, mu) /
                   GetTransmittanceToTopAtmosphereBoundary(TransmittanceTexture, r_d, mu_d), 
                   vec3(1));
    }
}

vec3 GetTransmittanceToSun(in sampler2D TransmittanceTexture, float r, float mu_s)
{
    float SinThetaH = Atmosphere.BottomRadius / r;
    float CosThetaH = -sqrt(max(1.0 - SinThetaH * SinThetaH, 0.0));
    return vec3(GetTransmittanceToTopAtmosphereBoundary(TransmittanceTexture, r, mu_s)) *
            smoothstep(-SinThetaH * Atmosphere.SunAngularRadius,
                       SinThetaH * Atmosphere.SunAngularRadius,
                       mu_s * CosThetaH);
}

vec3 ComputeDirectIrradiance(in sampler2D TransmittanceTexture, float r, float mu_s)
{
    float alpha_s = Atmosphere.SunAngularRadius;
    float avg_cos_factor = mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s : (mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));

    return Atmosphere.SolarIrradiance * GetTransmittanceToTopAtmosphereBoundary(TransmittanceTexture, r, mu_s) * avg_cos_factor;
}

vec3 ComputeDirectIrradianceTexture(in sampler2D TransmittanceTexture, in vec2 FragCoord)
{
    vec2 TexSize = vec2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
    float r, mu_s;
    GetRMuSFromIrradianceTextureUV(FragCoord / TexSize, r, mu_s);
    return ComputeDirectIrradiance(TransmittanceTexture, r, mu_s);
}

void ComputeSingleScatteringIntegrand(in sampler2D TransmittanceTexture, float r, float mu, float mu_s, float nu, float d, bool IntersectsGround, out vec3 Rayleigh, out vec3 Mie)
{
    float r_d = ClampRadius(sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);
    vec3 Transmittance = GetTransmittance(TransmittanceTexture, r, mu, d, IntersectsGround) * GetTransmittanceToSun(TransmittanceTexture, r_d, mu_s_d);
    Rayleigh = Transmittance * GetProfileDensity(Atmosphere.RayleighDensity, r_d - Atmosphere.BottomRadius);
    Mie = Transmittance * GetProfileDensity(Atmosphere.MieDensity, r_d - Atmosphere.BottomRadius);
}

void ComputeSingleScattering(in sampler2D TransmittanceTexture, float r, float mu, float mu_s, float nu, bool IntersectsGround, out vec3 Rayleigh, out vec3 Mie)
{
    int SAMPLE_COUNT = 50;
    float dx = DistanceToNearestAtmosphereBoundary(r, mu, IntersectsGround) / float(SAMPLE_COUNT);
    vec3 RayleighSum = vec3(0);
    vec3 MieSum = vec3(0);
    for(int i = 0; i <= SAMPLE_COUNT; ++i)
    {
        float d_i = float(i) * dx;
        vec3 R_i, M_i;
        ComputeSingleScatteringIntegrand(TransmittanceTexture, r, mu, mu_s, nu, d_i, IntersectsGround, R_i, M_i);
        float w_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
        RayleighSum += R_i * w_i;
        MieSum += M_i * w_i;
    }
    Rayleigh = RayleighSum * dx * Atmosphere.SolarIrradiance * Atmosphere.RayleighScattering;
    Mie = MieSum * dx * Atmosphere.SolarIrradiance * Atmosphere.MieExtinction;
}

void ComputeSingleScatteringTexture(in sampler2D TransmittanceTexture, in vec3 FragCoord, out vec3 Rayleigh, out vec3 Mie)
{
    float r, mu, mu_s, nu;
    bool IntersectsGround;
    GetRMuMuSNuFromScatteringTextureFragCoord(FragCoord, r, mu, mu_s, nu, IntersectsGround);
    ComputeSingleScattering(TransmittanceTexture, r, mu, mu_s, nu, IntersectsGround, Rayleigh, Mie);
}

void main()
{
    if(ProgramUnit == 0)
    { // Transmittance texture
        out0 = vec4(ComputeTransmittanceToTopAtmosphereBoundaryTexture(gl_FragCoord.xy), 1);
    }
    else if(ProgramUnit == 1)
    { // Ground Irradiance texture
        out0 = vec4(ComputeDirectIrradianceTexture(Tex0, gl_FragCoord.xy), 1); // delta irradiance
        out1 = vec4(0.0); // irradiance (nothing yet, no direct)
    }
    else if(ProgramUnit == 2)
    { // Single Scattering
        vec3 R, M;
        ComputeSingleScatteringTexture(Tex0, vec3(gl_FragCoord.xy, ScatteringLayer + 0.5), R, M);
        out0 = vec4(R, 1.0); // delta raleigh
        out1 = vec4(M, 1.0); // delta mie
        out2 = vec4(R, M.r); // scattering
    }
    else
    {
        out0 = vec4(1,0,1, 1);
    }
}
