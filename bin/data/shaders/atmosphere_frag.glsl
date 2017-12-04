#version 400
#define PI 3.14159265359

// TODO - give them as uniform from program constants, or create the shader from the code with this hardcoded
#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

#define IRRADIANCE_TEXTURE_WIDTH 64
#define IRRADIANCE_TEXTURE_HEIGHT 16

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
};


in vec2 v_texcoord;
in vec3 v_eyeRay;

uniform atmosphere_parameters Atmosphere;
uniform vec3 CameraPosition;
uniform vec3 SunDirection;
uniform sampler2D TransmittanceTexture;
uniform sampler2D IrradianceTexture;

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

#if 0


void ComputeSingleScatteringIntegrand(in atmosphere_parameters atmosphere, float r, float mu, float mu_s, float nu, float d, bool IntersectsGround, out vec3 Rayleigh, out vec3 Mie)
{
    float r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);
    vec3 Transmittance = GetTransmittance(atmosphere, r, mu, d, IntersectsGround) * GetTransmittanceToSun(atmosphere, r_d, mu_s_d);
    Rayleigh = Transmittance * GetProfileDensity(atmosphere.RayleighDensity, r_d - atmosphere.BottomRadius);
    Mie = Transmittance * GetProfileDensity(atmosphere.MieDensity, r_d - atmosphere.BottomRadius);
}

void ComputeSingleScattering(in atmosphere_parameters atmosphere, float r, float mu, float mu_s, float nu, bool IntersectsGround, out vec3 Rayleigh, out vec3 Mie)
{
    int SAMPLE_COUNT = 30;
    float dx = DistanceToNearestAtmosphereBoundary(atmosphere, r, mu, IntersectsGround) / float(SAMPLE_COUNT);
    vec3 RayleighSum = vec3(0);
    vec3 MieSum = vec3(0);
    for(int i = 0; i <= SAMPLE_COUNT; ++i)
    {
        float d_i = float(i) * dx;
        vec3 R_i, M_i;
        ComputeSingleScatteringIntegrand(atmosphere, r, mu, mu_s, nu, d_i, IntersectsGround, R_i, M_i);
        float w_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
        RayleighSum += R_i * w_i;
        MieSum += M_i * w_i;
    }
    Rayleigh = RayleighSum * dx * atmosphere.SolarIrradiance * atmosphere.RayleighScattering;
    Mie = MieSum * dx * atmosphere.SolarIrradiance * atmosphere.MieExtinction;
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

vec3 ComputeCombinedScattering(in atmosphere_parameters atmosphere, float r, float mu, float mu_s, float nu, bool IntersectsGround, out vec3 Mie)
{
    float rmu = r * mu;
    float d = -rmu - sqrt(rmu * rmu - r * r + atmosphere.TopRadius * atmosphere.TopRadius);
    float r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu * d) / r_d);

    Mie = vec3(0);
    vec3 contrib = vec3(0);
    contrib = min(ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu) /
            ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r_d, mu_d), vec3(1));
    //vec3 R, M;
    //ComputeSingleScattering(atmosphere, r, mu, mu_s, nu, IntersectsGround, R, M);
    //contrib += R;
    //Mie += M;
    return contrib;
}


#endif
vec3 GetSolarRadiance()
{
    return Atmosphere.SolarIrradiance / (PI * Atmosphere.SunAngularRadius * Atmosphere.SunAngularRadius);
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

    vec3 contrib = vec3(0);
    float mu = rmu / r;
    float mu_s = dot(P, L) / r;
    float nu = max(0.0,dot(E, L));

    float r_d = ClampRadius(sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu * d) / r_d);
    bool IntersectsGround = RayIntersectsGround(r, mu);

    Transmittance = min(vec3(1), GetTransmittance(r, mu, d, IntersectsGround));
    contrib += Transmittance;
    //contrib += min(ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu) /
            //ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r_d, mu_d), vec3(1));
    //vec3 MieScattering;
    //contrib += ComputeCombinedScattering(atmosphere, r, mu, mu_s, nu, IntersectsGround, MieScattering);

    return contrib;// * RayleighPhaseFunction(nu) + MieScattering * MiePhaseFunction(atmosphere.MiePhaseG, nu);
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
    // TODO - single & multiple scattering
    return vec3(0);
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
    vec3 p = CameraPosition - EarthCenter;
    float PdotV = dot(p, E);
    float PdotP = dot(p, p);
    float EarthCenterRayDistance = PdotP - PdotV * PdotV;
    float Depth = -PdotV - sqrt(Atmosphere.BottomRadius*Atmosphere.BottomRadius - EarthCenterRayDistance);

    vec3 L = normalize(SunDirection);

    vec3 contrib = vec3(0);
    if(Depth > 0.0)
    {
        vec3 Point = CameraPosition + E * Depth;
        vec3 N = normalize(Point - EarthCenter);
        Point += N * 1e-3; // To avoid depth fighting

        vec3 SkyIrradiance;
        vec3 SunIrradiance = GetGroundIrradiance(Point - EarthCenter, N, L, SkyIrradiance);
        vec3 GroundRadiance = Atmosphere.GroundAlbedo * (1.0/PI) * (SunIrradiance + SkyIrradiance);

        vec3 Transmittance;
        vec3 Inscattering = GetSkyRadianceToPoint(p, Point - EarthCenter, L, Transmittance);
        contrib += GroundRadiance * Transmittance + Inscattering;
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

    #if 0
    if(Depth > 0.0)
    {

        vec3 SunRadiance = GetGroundIrradiance(Atmosphere, Point - EarthCenter, N, L);
        contrib += 1e-4 * SunRadiance * Atmosphere.GroundAlbedo * (1.0/PI);
    }
    else
    {
    }
    #endif

    frag_color = vec4(contrib, 1); 
}
