#version 400
#define PI 3.14159265359

// TODO - give them as uniform from program constants, or create the shader from the code with this hardcoded
#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

#define IRRADIANCE_TEXTURE_WIDTH 64
#define IRRADIANCE_TEXTURE_HEIGHT 16

#define SCATTERING_TEXTURE_R_SIZE 64
#define SCATTERING_TEXTURE_MU_SIZE 128
#define SCATTERING_TEXTURE_MU_S_SIZE 32
#define SCATTERING_TEXTURE_NU_SIZE 8

#define SCATTERING_TEXTURE_WIDTH (SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE)
#define SCATTERING_TEXTURE_HEIGHT SCATTERING_TEXTURE_MU_SIZE
#define SCATTERING_TEXTURE_DEPTH SCATTERING_TEXTURE_R_SIZE

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

const vec3 kGroundAlbedo = vec3(0.0005, 0.001, 0.005);

in vec2 v_texcoord;
in vec3 v_eyeRay;

uniform atmosphere_parameters Atmosphere;
uniform vec3 CameraPosition;
uniform vec3 SunDirection;
uniform sampler2D TransmittanceTexture;
uniform sampler2D IrradianceTexture;
uniform sampler3D ScatteringTexture;

out vec4 frag_color;

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

bool RayIntersectsGround(float r, float mu)
{
    return mu < 0.0 && (r * r * (mu * mu - 1.0) + Atmosphere.BottomRadius * Atmosphere.BottomRadius) >= 0.0;
}

float GetTextureCoordFromUnitRange(float x, int TexSize)
{
    return 0.5 / float(TexSize) + x * (1.0 - 1.0 / float(TexSize));
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

vec2 GetIrradianceTextureUVFromRMuS(float r, float mu_s)
{
    float x_r = (r - Atmosphere.BottomRadius) / (Atmosphere.TopRadius - Atmosphere.BottomRadius);
    float x_mu_s = mu_s * 0.5 + 0.5;
    return vec2(GetTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH), GetTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
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

vec3 GetTransmittanceToTopAtmosphereBoundary(float r, float mu)
{
    vec2 uv = GetTransmittanceTextureUVFromRMu(r, mu);
    return texture(TransmittanceTexture, uv).xyz;
}

vec3 GetTransmittance(float r, float mu, float d, bool IntersectsGround)
{
    float r_d = ClampRadius(sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu + d) / r_d);

    if(IntersectsGround)
    {
        return min(GetTransmittanceToTopAtmosphereBoundary(r_d, -mu_d) /
                   GetTransmittanceToTopAtmosphereBoundary(r, -mu), 
                   vec3(1));
    }
    else
    {
        return min(GetTransmittanceToTopAtmosphereBoundary(r, mu) /
                   GetTransmittanceToTopAtmosphereBoundary(r_d, mu_d), 
                   vec3(1));
    }
}

vec3 GetTransmittanceToSun(float r, float mu_s)
{
    float SinThetaH = Atmosphere.BottomRadius / r;
    float CosThetaH = -sqrt(max(1.0 - SinThetaH * SinThetaH, 0.0));
    return vec3(GetTransmittanceToTopAtmosphereBoundary(r, mu_s)) *
            smoothstep(-SinThetaH * Atmosphere.SunAngularRadius,
                       SinThetaH * Atmosphere.SunAngularRadius,
                       mu_s * CosThetaH);
}

vec3 GetIrradiance(float r, float mu_s)
{
    vec2 uv = GetIrradianceTextureUVFromRMuS(r, mu_s);
    return texture(IrradianceTexture, uv).xyz;
}

float RayleighPhaseFunction(float nu)
{
    float k = 3.0 / (16.0 * PI);
    return k * (1.0 + nu * nu);
}

float MiePhaseFunction(float g, float nu)
{
    float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

vec3 GetSolarRadiance()
{
    return Atmosphere.SolarIrradiance / (PI * Atmosphere.SunAngularRadius * Atmosphere.SunAngularRadius);
}

vec3 GetExtrapolatedSingleMieScattering(in vec4 Scattering)
{
    if(Scattering.r == 0.0) return vec3(0.0);
    return Scattering.rgb * Scattering.a / Scattering.r * (Atmosphere.RayleighScattering.r / Atmosphere.MieScattering.r) *
            (Atmosphere.MieScattering / Atmosphere.RayleighScattering);
}

vec3 GetScattering(float r, float mu, float mu_s, float nu, bool IntersectsGround, out vec3 Mie)
{
    vec4 uvwz = GetScatteringTextureUVWZFromRMuMuSNu(r, mu, mu_s, nu, IntersectsGround);
    float texCoordX = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
    float texX = floor(texCoordX);
    float lerp = texCoordX - texX;
    vec3 uvw0 = vec3((texX + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);
    vec3 uvw1 = vec3((texX + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);

    vec4 CombinedScattering = (texture(ScatteringTexture, uvw0) * (1.0 - lerp) + 
                               texture(ScatteringTexture, uvw1) * lerp);
    Mie = GetExtrapolatedSingleMieScattering(CombinedScattering);

    return CombinedScattering.rgb;
}

vec3 GetSkyRadiance(vec3 P, vec3 E, out vec3 Transmittance)
{
    vec3 L = normalize(SunDirection);
    float r = length(P);
    float rmu = dot(P, E);

    float d = -rmu - sqrt(rmu * rmu - r * r + Atmosphere.TopRadius * Atmosphere.TopRadius);
    if(d > 0.0)
    { // if outside atmosphere looking in, move along the ray to the top boundary
        P = P + E * d;
        r = Atmosphere.TopRadius;
        rmu += d;
    }
    else if(r > Atmosphere.TopRadius)
    { // If we are outside the atmosphere and not looking in, return 0 (space)
        return vec3(0);
    }

    float mu = rmu / r;
    float mu_s = dot(P, L) / r;
    float nu = max(0.0,dot(E, L));

    float r_d = ClampRadius(sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu * d) / r_d);
    bool IntersectsGround = RayIntersectsGround(r, mu);

    Transmittance = IntersectsGround ? vec3(0.0) : GetTransmittanceToTopAtmosphereBoundary(r, mu);
    vec3 Rayleigh, Mie;
    Rayleigh = GetScattering(r, mu, mu_s, nu, IntersectsGround, Mie);

    float Flattening =  max(1e-5,L.y); // Sun
    //float Flattening =  1.f; // Moon
    return Flattening * (Rayleigh * RayleighPhaseFunction(nu) + Mie * MiePhaseFunction(Atmosphere.MiePhaseG, nu));
}

vec3 GetSkyRadianceToPoint(vec3 Camera, vec3 P, vec3 L, out vec3 Transmittance)
{
    vec3 ViewRay = normalize(P - Camera);
    float r = length(Camera);
    float rmu = dot(Camera, ViewRay);
    float DistToTop = -rmu - sqrt(rmu * rmu - r * r + Atmosphere.TopRadius * Atmosphere.TopRadius);
    if(DistToTop > 0.0)
    {
        Camera = Camera + ViewRay * DistToTop;
        r = Atmosphere.TopRadius;
        rmu += DistToTop;
    }

    float mu = rmu / r;
    float mu_s = dot(Camera, L) / r;
    float nu = dot(ViewRay, L);
    float d = length(P - Camera);
    bool IntersectsGround = RayIntersectsGround(r, mu);

    Transmittance = GetTransmittance(r, mu, d, IntersectsGround);
    vec3 Rayleigh, Mie;
    Rayleigh = GetScattering(r, mu, mu_s, nu, IntersectsGround, Mie);

    float r_p = ClampRadius(sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_p = (r * mu + d) / r_p;
    float mu_s_p = (r * mu_s + d * nu) / r_p;

    vec3 Rayleigh_p, Mie_p;
    Rayleigh_p = GetScattering(r_p, mu_p, mu_s_p, nu, IntersectsGround, Mie_p);

    Rayleigh = Rayleigh - Transmittance * Rayleigh_p;
    Mie = Mie - Transmittance * Mie_p;
    Mie = GetExtrapolatedSingleMieScattering(vec4(Rayleigh, Mie.r));

    Mie = Mie * smoothstep(0.0, 0.01, mu_s);

    return Rayleigh * RayleighPhaseFunction(nu) + Mie * MiePhaseFunction(Atmosphere.MiePhaseG, nu);
}

vec3 GetGroundIrradiance(vec3 P, vec3 N, vec3 L, out vec3 SkyIrradiance)
{
    float r = length(P);
    float mu_s = dot(P, L) / r;

    SkyIrradiance = GetIrradiance(r, mu_s) * (1.0 + dot(N, P) / r) * 0.5;

    return Atmosphere.SolarIrradiance * max(0.0, dot(N, L)) * GetTransmittanceToSun(r, mu_s);
}

void main()
{
    vec3 E = normalize(v_eyeRay);
    vec3 EarthCenter = vec3(0,-Atmosphere.BottomRadius, 0);
    vec3 P = CameraPosition;
    vec3 p = P - EarthCenter;
    float PdotV = dot(p, E);
    float PdotP = dot(p, p);
    float EarthCenterRayDistance = PdotP - PdotV * PdotV;
    float Depth = -PdotV - sqrt(Atmosphere.BottomRadius*Atmosphere.BottomRadius - EarthCenterRayDistance);

    vec3 L = normalize(SunDirection);

    vec3 contrib = vec3(0);
    if(Depth > 0.0)
    {
        vec3 Point = P + E * Depth;
        vec3 N = normalize(Point - EarthCenter);
        Point += N * 1e-3f; // To avoid depth fighting

        vec3 SkyIrradiance;
        vec3 SunIrradiance = GetGroundIrradiance(Point - EarthCenter, N, L, SkyIrradiance);
        vec3 GroundRadiance = kGroundAlbedo * (1.0/PI) * (SunIrradiance + SkyIrradiance);

        vec3 Transmittance;
        vec3 Inscattering = GetSkyRadianceToPoint(p, Point - EarthCenter, L, Transmittance);
        contrib += Transmittance * GroundRadiance + Inscattering;
    }
    else
    {
        vec3 Transmittance;
        contrib += max(vec3(0), GetSkyRadiance(p, E, Transmittance));
        if(dot(E, L) > cos(Atmosphere.SunAngularRadius))
        {
            contrib += Transmittance * GetSolarRadiance();
        }
    }

    frag_color = vec4(contrib, 1); 
}
