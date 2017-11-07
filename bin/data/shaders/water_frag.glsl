#version 400

#define PI     3.14159265358
#define INV_PI 0.31830988618

in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_sundirection;

uniform samplerCube Skybox;
uniform samplerCube IrradianceCubemap;

uniform vec4 LightColor;
uniform vec3 CameraPos;
uniform float Time;

out vec4 frag_color;

// TODO - Constants, parameterize this
const float SigmaA = 0.00055;
const float SigmaS = 0.0000;
const float SigmaT = SigmaA + SigmaS;
const vec3 FogColor = 0.1*vec3(0.10, 0.20, 0.35);
const vec3 WaterColor = vec3(0.08, 0.15, 0.40);
const vec3 WaterCrestColor = vec3(0.80, 0.70, 0.90);
const vec3 AmbWaterColor = vec3(0.01, 0.10, 0.20);
const vec3 SSSColor = vec3(0, 0.1, 0.065);
const float SeaHeight = -10.0;
const float DiffusePart = 0.05;
const float SpecularPart = 4.0;
const float SpecularRoughness = 512;

const vec2 FractDelta = vec2(0.005, 0.0);
const mat2 OctaveMat = mat2(1.6, 1.2, -1.2, 1.6);
const vec2 WindSpeed = vec2(0.43,  0.13);
const vec2 WindDirection = normalize(vec2(1,0));
const vec2 WaveChopiness = vec2(0.81, 0.13);
const vec2 WaveInvScale = vec2(0.15, 0.25);

vec3 FresnelSchlick(float u, vec3 f0, float f90)
{
    return f0 + (vec3(f90) - f0) * pow(1 - u, 5);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    //float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
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

float DisneyFrostbite(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float eBias = mix(0.0, 0.5, linearRoughness);
    float eFactor = mix(1.0, 1.0 / 1.51, linearRoughness);
    float f90 = eBias + 2.0 * LdotH * LdotH * linearRoughness;
    vec3 f0 = vec3(1);
    float lightScatter = FresnelSchlick(NdotL, f0, f90).r;
    float viewScatter = FresnelSchlick(NdotV, f0, f90).r;
    return lightScatter * viewScatter * eFactor;
}

vec3 Shading(vec3 Pos, float water_dist, vec3 Rd, vec3 N, vec3 L)
{
    vec3 V = -Rd;
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);
    float NdotL = max(0.0, dot(N, L));
    float NdotV = min(1, abs(dot(N, V)) + 1e-1);
    float NdotH = max(0.0, dot(N, H));
    float LdotH = max(0.0, dot(L, H));

    vec3 Envmap = pow(texture(Skybox, -R).xyz, vec3(2.2));
    vec3 Irradiance = pow(texture(IrradianceCubemap, -R).xyz, vec3(2.2));
    float spec = pow(NdotH, SpecularRoughness);

    vec3 f0 = vec3(0.0001);
    float roughness = 0.999991;

    vec3 F = FresnelSchlick(LdotH, f0, 1);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float D = GeometrySchlickGGX(NdotH, roughness);

    vec3 light = vec3(0);

    // Diffuse
    #if 0
    float ks = F;
    float kd = 1 - ks;
    float fake_kd = min(1, 1.5 - ks);
    float fake_ks = 1 - fake_kd;
    #endif

    vec3 nom = F*D*G;
    float denom = (4.0 * NdotV * NdotL + 1e-4);

    vec3 Specular = nom * NdotL * WaterCrestColor / denom;
    vec3 Diffuse = DisneyFrostbite(NdotV, NdotL, LdotH, 1.0-roughness) * WaterColor * NdotL / PI;

    float ks = FresnelSchlick(min(1, abs(dot(N,V))+1e-1), f0, roughness).x;
    float kd = 1.0 - ks;
    vec3 ambient_diffuse = kd * Irradiance * WaterColor;
    //vec3 ambient_specular = (1-min(1, 1.3-ks)) * WaterCrestColor.xyz;
    vec3 ambient_specular = min(1, 1.3-ks) * WaterCrestColor.xyz;
    vec3 Ambient = ambient_specular + ambient_diffuse;

    light = Ambient + LightColor.xyz * (Specular);

    //if(F > 0.0)
    //{
    #if 0
        vec3 env_light = mix(Irradiance, Envmap, 0.07 * exp(-water_dist * 0.001));//0.085);
        light += F*G*D * NdotL / (4.0 * NdotV * NdotL + 1e-4) * env_light * FCol;
        light += spec * SpecularPart * kd * LightColor.xyz;

        // fake SSS
        light += kd * SSSColor * NdotV * NdotV * NdotV * pow(max(0.0, dot(V, -L)), 5) * max(0.0, 0.5 - max(0.0, dot(V, vec3(0, 1, 0))));
        light += kd * 0.04 * AmbWaterColor;
    #else

        //light += F*D*G * NdotL / (4.0 * NdotV * NdotL + 1e-4) * FCol;
        //light += fake_kd * WaterColor;
        //light += fake_ks * WaterCrestColor.xyz;
        light += kd * LightColor.xyz * SSSColor * pow(max(0.0, dot(V, -L)), 5)* NdotV * NdotV * NdotV;
    #endif
    //}
    return light;
}

vec3 Fog(vec3 rgb, vec3 Rd, vec3 L, vec3 light_rgb, float dist, float sigma_t)
{
    float fog_amount = 1.0 - exp(-dist * sigma_t);
    return mix(rgb, light_rgb, fog_amount);
}

float Hash(in float n)
{
    return fract(sin(n) * 43758.5453123);
}

void Hash2D(vec2 Cell, out vec4 Hx, out vec4 Hy)
{ // NOTE - FAST32_hash
    const vec2 hash_offset = vec2(26.0, 161.0);
    const float hash_domain = 71.0;
    const vec2 hash_large = vec2(878.124879, 588.154868);
    vec4 P = vec4(Cell.xy, Cell.xy + 1.0);
    P = P - floor(P * (1.0/hash_domain)) * hash_domain;
    P += hash_offset.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    Hx = fract(P * (1.0/hash_large.x));
    Hy = fract(P * (1.0/hash_large.y));
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

vec2 InterpC2(vec2 x)
{
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

float Perlin2D(vec2 P)
{
    vec2 Pi = floor(P);
    vec4 dP = P.xyxy - vec4(Pi, Pi + 1.0);
    vec4 hx, hy;
    Hash2D(Pi, hx, hy);

    vec4 dx = hx - 0.49999;
    vec4 dy = hy - 0.49999;
    vec4 dr = inversesqrt(dx * dx + dy * dy) * (dx * dP.xzxz + dy * dP.yyww);

    dr *= 1.4142135623730950488016887242097;
    vec2 blend = InterpC2(dP.xy);
    vec4 blend2 = vec4(blend, vec2(1.0 - blend));
    return dot(dr, blend2.zxzx * blend2.wwyy);
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

void main()
{

    vec3 Rd = v_position - CameraPos;
    float water_dist = length(Rd);
    if(water_dist <= 0) discard;
    Rd /= water_dist;

    vec3 L = normalize(v_sundirection);
    vec3 N = normalize(v_normal);

    vec3 FractN = GetFractalNormal(v_position, N);

    // Blending details into normal
    FractN = mix(FractN, N, 1.0 - exp(-water_dist * 0.005));
    FractN = normalize(FractN);


    vec3 shading = Shading(v_position, water_dist, Rd, FractN, L);
    vec3 color = Fog(shading, Rd, L, FogColor, water_dist, SigmaT);

    frag_color = vec4(color, 1);
}

#if 0
// BACKUP UNDERWATER CODE
    {
        // Underwater params
        float sigma_a_uw = 0.01;
        float sigma_s_uw = 0.00;
        float sigma_t_uw = sigma_a_uw + sigma_s_uw;
        vec3 uw_far_color = vec3(0.0075, 0.15, 0.23);
        vec3 uw_close_color = vec3(0.2, 0.7, 0.8);

        float uwNdotL = max(0, dot(L, N));
        float uwNdotV = max(0, dot(V, -N));
        float uwVdotInvL2 = pow(max(0, dot(L, -V)), 2);
        vec3 Ri = refract(V, N, 0.99);
        vec3 refract_color = texture(Skybox, Ri).xyz;
        vec3 refraction = (1 - refract_ratio) * vec3(1) + refract_ratio * refract_color;

        float dist = exp(-sigma_t_uw * water_dist);

        color = (1-dist) * uw_far_color;
        //color += dist * vec4(FresnelSchlick(refraction.xyz * sky_color.xyz, water_color.xyz * nSnell, uwNdotV),1);
        //frag_color.a = 1.0;//0.8 + 0.2 * (1-FresnelRatio(NdotV * nSnell));
    }
#endif
