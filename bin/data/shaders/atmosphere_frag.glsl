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

const vec3 kGroundAlbedo = vec3(0.125, 0.15, 0.2);

in vec2 v_texcoord;
in vec3 v_eyeRay;
in vec3 v_sunUV;
in vec3 v_CameraPosition;

uniform atmosphere_parameters Atmosphere;
uniform vec3 SunDirection;
uniform sampler2D TransmittanceTexture;
uniform sampler2D IrradianceTexture;
uniform sampler3D ScatteringTexture;
uniform sampler2D MoonAlbedo;
uniform float Time;
uniform vec2 Resolution;

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
                       mu_s - CosThetaH);
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
    bool IntersectsGround = false;//RayIntersectsGround(r, mu);

    Transmittance = IntersectsGround ? vec3(0.0) : GetTransmittanceToTopAtmosphereBoundary(r, mu);
    vec3 Rayleigh, Mie;
    Rayleigh = GetScattering(r, mu, mu_s, nu, IntersectsGround, Mie);

    return (Rayleigh * RayleighPhaseFunction(nu) + Mie * MiePhaseFunction(Atmosphere.MiePhaseG, nu));
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
    bool IntersectsGround = true;//RayIntersectsGround(r, mu);

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

float atan2(in float y, in float x)
{
    return x == 0.0 ? sign(y)*PI/2.0 : atan(y,x);
}

const vec2 FractDelta = vec2(1.01, 0.0);
const mat2 OctaveMat = mat2(1.4, 1.2, -1.2, 1.4);
const vec2 WindSpeed = vec2(0.33,  0.13);
const vec2 WaveChopiness = vec2(0.21, 0.13);
const vec2 WaveInvScale = vec2(0.05, 0.005);

float Hash(in float n)
{
    return fract(sin(n) * 43758.5453123);
}

float Noise(in vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0;
    float res = mix(mix(Hash(n + 0.0), Hash(n + 1.0), f.x),
                    mix(Hash(n +57.0), Hash(n +58.0), f.x), f.y);
    return res;
}

float FractalNoise(in vec2 xy)
{
    float theta = PI/2.f;
    float m = 1.25;
    float w = 0.6;
    float f = 0.0;
    for(int i = 0; i < 6; ++i)
    {
        f += Noise(WaveInvScale.x * xy + Time * WindSpeed.x) * m * WaveChopiness.x;
        if(i < 2)
        {
            //f += Perlin2D(WaveInvScale.y * xy.xy + Time * WindSpeed.y) * w * WaveChopiness.y;
        }
        else
        {
            //f += abs(Perlin2D(WaveInvScale.y * xy.xy + Time * WindSpeed.y) * w * WaveChopiness.y);
        }
        w *= 0.45;
        m *= 0.35;
        xy *= OctaveMat;
    }
    return f * (abs(sin(1.0-Time * 0.025)) * 0.25 + 0.75);
}

float Dist(in vec3 Pos, in vec3 N)
{
    return dot(Pos - vec3(0, -FractalNoise(Pos.xz), 0), N);
}

vec3 GetFractalNormal(in vec3 Pos, in vec3 N)
{
    vec3 n;
    n.x = Dist(Pos + FractDelta.xyy, N) - Dist(Pos - FractDelta.xyy, N);
    n.y = Dist(Pos + FractDelta.yxy, N) - Dist(Pos - FractDelta.yxy, N);
    n.z = Dist(Pos + FractDelta.yyx, N) - Dist(Pos - FractDelta.yyx, N);
    return normalize(n);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlickRoughness(float u, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1-roughness), f0) - f0) * pow(1 - u, 5);
}

float regShape(vec2 p, int N)
{
    float a = atan(p.x, p.y) + 0.2;
    float b = 6.28319/float(N);
    float f = smoothstep(0.5, 0.51, cos(floor(0.5 + a/b) * b - a) * length(p.xy));
    return f;
}

vec3 circle(vec2 p, float size, float decay, vec3 color, float dist, vec2 sp)
{
    float len = length(p + sp * (dist*4.0));
    float l = len + size/2.0;
    float l2 = len + size/3.0;
    float c = max(0.01 - pow(length(p + sp*dist), size*1.0), 0.0) * 50.0;
    //float ring = max(0.001 - pow(l - 0.5, 0.02) + sin(l*20.0), 0.0) * 3.0;
    float dots = max(0.03/pow(length(p-sp*dist*0.5)*1.0, 1.0), 0.0) / 20.0;
    float hex = max(0.02-pow(regShape(p*5.0 + sp*dist*1.0, 6), 1.0), 0.0) * 5.0;

    color = 0.5 + 0.5 * sin(color);
    color = cos(vec3(0.44, 0.24, 0.2) * 8.0 + dist * 4.0) * 0.5 + 0.5;

    vec3 f = c * color;
    //f += ring * color;
    f += dots * color;
    f += hex * color;

    return f - 0.01;
}

float rnd(float w)
{
    return fract(sin(w) * 1000.0);
}

vec3 AddSunGlare(vec3 Color, vec3 SunColor, vec3 E, vec3 L)
{
    float AspectRatio = Resolution.x / Resolution.y;
    vec2 uv = v_texcoord - vec2(0.5);
    uv.x *= AspectRatio;

    float sx = 0.5*(v_sunUV.x+1);
    float sy = 0.5*(v_sunUV.y+1);
    vec2 uvSun = vec2(sx, sy) - vec2(0.5);
    uvSun.x *= AspectRatio;

    float viewFalloff = max(0.0, dot(E, L));
    float horizonFalloff = max(0.0, 1.0 - exp(-1000.0*min(1.0, L.y + 1.1*Atmosphere.SunAngularRadius)));
    uvSun.y += (1.0 - horizonFalloff) * Atmosphere.SunAngularRadius * 1.1;

    vec3 Glare = vec3(0);

    for(float i = 0; i < 5; ++i)
    {
        Glare += 0.0001 * circle(uv, pow(rnd(i*4000.0), 2.0) + 1.41, 0.0, SunColor, rnd(i*2.0)*3.0+0.2-0.5, uvSun);
    }

    float a = atan(uv.y-uvSun.y, uv.x-uvSun.x);
    // Sun fake shafts
    Glare += max(0.000005/pow(length(uv-uvSun)*8.0, 2.0),0.0) * abs(sin(a*3.+cos(a*9.))) * abs(sin(a*9.));
    // Sun glow
    Glare += max(0.000001/pow(length(uv-uvSun), 2.0),0.0);//*vec3(0.3,0.21,0.1);

    // Sun color, camera falloff and horizon falloff
    Color += Glare * SunColor * viewFalloff * horizonFalloff;

    return Color;
}

vec3 WaterShading(in vec3 P, in float depth, in vec3 N, in vec3 E, in vec3 L, in vec3 SkyRadiance, in vec3 SunRadiance, in vec3 WaterColor)
{
    vec3 shading;
    vec3 V = -E;
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);

    float HdotV = max(0.0, dot(H, V));
    float NdotL = max(0.0, dot(N, L));
    float NdotV = max(0.0, min(1.0, dot(N, V)));
    float NdotH = max(0.0, dot(N, H));

    vec3 f0 = vec3(0.22);
    float roughness = 0.22;

    vec3 F = FresnelSchlickRoughness(NdotV, f0, roughness);
    //F = mix(F, vec3(1.0), 1.0-exp(-depth * 1.0)); // depth 
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float D = DistributionGGX(NdotH, roughness);

    vec3 ks = F;
    vec3 kd = vec3(1.0) - ks;

    vec3 nom = F * D * G;
    float denom = (4.0 * NdotV * NdotL + 1e-5);

    vec3 Specular = nom / denom;
    vec3 Diffuse = kd * WaterColor / PI;
    shading = (Specular + Diffuse) * (max(0.0, L.y));

    ks = FresnelSchlickRoughness(NdotV, f0, 0.6);
    vec3 Ambient = ks * SkyRadiance;


    return shading * SkyRadiance + Ambient;
}

void main()
{
    vec3 contrib = vec3(0);
    contrib = vec3(1,0,0) * 10000;
    if(gl_PrimitiveID == 1)
        contrib = vec3(gl_FragCoord.z) * 10000;
#if 0
    vec3 E = normalize(v_eyeRay);
    vec3 EarthCenter = vec3(0,-Atmosphere.BottomRadius, 0);
    vec3 P = v_CameraPosition;
    vec3 p = P - EarthCenter;
    float PdotV = dot(p, E);
    float PdotP = dot(p, p);
    float EarthCenterRayDistance = PdotP - PdotV * PdotV;
    float Depth = -PdotV - sqrt(Atmosphere.BottomRadius*Atmosphere.BottomRadius - EarthCenterRayDistance);

    vec3 L = normalize(SunDirection);
    float EdotL = dot(E,L);

    vec3 Transmittance;
    vec3 SolarRadiance = GetSolarRadiance();

    float r = length(P);
    float mu = PdotV / r;

    if(Depth > 0)
    { // earth
        vec3 Point = P + E * Depth;
        vec3 N = normalize(Point - EarthCenter);

        vec3 ShadingPoint = 1000*Point; // ajusted for km scale
        vec3 FractN = GetFractalNormal(ShadingPoint, N);
        FractN = mix(FractN, N, 1.0 - exp(-Depth * 1.0));
        FractN = normalize(FractN);

        vec3 SkyIrradiance;
        vec3 SunIrradiance = GetGroundIrradiance(Point - EarthCenter, FractN, L, SkyIrradiance);
        vec3 GroundRadiance = kGroundAlbedo * (1.0/PI) * (SunIrradiance + SkyIrradiance);

        vec3 Inscattering = GetSkyRadianceToPoint(p, Point - EarthCenter, L, Transmittance);
        //contrib = GroundRadiance * Transmittance + Inscattering*50;
        vec3 ReflectedSkyRadiance = max(vec3(0), GetSkyRadiance(Point-EarthCenter,reflect(E,FractN),Transmittance));
        contrib = Inscattering * 2.0 + 
                  WaterShading(ShadingPoint, Depth, FractN, E, L, ReflectedSkyRadiance, ReflectedSkyRadiance, kGroundAlbedo);
    }
    else
    { // sky
        contrib += pow(max(vec3(0), GetSkyRadiance(p, E, Transmittance)), vec3(1.00));
        float mu = cos(Atmosphere.SunAngularRadius);

        if(EdotL > mu)
        {
            contrib += Transmittance * SolarRadiance;

            /*
            // Map the moon albedo on the moon representation
            float moon_distance = 384400;
            vec3 L_normed = L * moon_distance;
            float moon_radius = moon_distance * sin(Atmosphere.SunAngularRadius);
            float t_ca = dot(L_normed, E);
            float d = sqrt(moon_distance*moon_distance - t_ca*t_ca);
            float t_hc = sqrt(moon_radius*moon_radius - d*d);
            float t_0 = t_ca - t_hc;
            vec3 moon_center = L_normed;
            vec3 moon_intersect = E * t_0;
            vec3 M = -normalize(moon_intersect - moon_center);
            vec2 spherical_uv;
            spherical_uv.x = ((atan2(M.z,M.x) + PI)*0.5) / (PI);
            spherical_uv.y = (M.y + 1.0) * 0.5;

            vec3 moon_albedo = pow(texture(MoonAlbedo, spherical_uv).xyz, vec3(2.2));
            contrib *= moon_albedo;
            */
        }
    }

    // Get the sun radiance modulated by the transmittance color
    SolarRadiance *= Transmittance / length(Transmittance);

    contrib = AddSunGlare(contrib, SolarRadiance, E, L);
    #endif

    frag_color = vec4(contrib, 1); 
}
