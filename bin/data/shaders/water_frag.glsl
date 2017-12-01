#version 400

#define PI     3.14159265358
#define INV_PI 0.31830988618
#define MAX_REFLECTION_LOD 4.0

in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_sundirection;

uniform sampler2D GGXLUT;
uniform samplerCube Skybox;

uniform vec4 LightColor;
uniform vec3 CameraPos;
uniform float Time;

out vec4 frag_color;

// TODO - Constants, parameterize this
const float SigmaA = 0.00025;
const float SigmaS = 0.0000;
const float SigmaT = SigmaA + SigmaS;
const vec3 FogColor = vec3(0.10, 0.17, 0.35);
const vec3 WaterColor = vec3(0.01, 0.03, 0.06);
const vec3 WaterCrestColor = vec3(1);//vec3(0.80, 0.70, 0.90);
const vec3 AmbWaterColor = vec3(0.01,0.05,0.2);//vec3(0.16, 0.30, 0.40);
const vec3 SSSColor = vec3(0, 0.40, 0.260);
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


vec3 FresnelSchlick(float u, vec3 f0)
{
    return f0 + (vec3(1.0) - f0) * pow(1 - u, 5);
}

vec3 FresnelSchlickRoughness(float u, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1-roughness), f0) - f0) * pow(1 - u, 5);
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

vec3 Shading(vec3 Pos, float water_dist, vec3 Rd, vec3 N, vec3 L)
{
    vec3 V = -Rd;
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);
    float NdotL = max(0.0, dot(N, L));
    float NdotV = max(0.0, min(1.0,1e-1+dot(N, V)));
    float NdotH = max(0.0, dot(N, H));
    float LdotH = max(0.0, dot(L, H));
    float HdotV = max(0.0, min(1,dot(H, V)));

    vec3 Irradiance = textureLod(Skybox, N, MAX_REFLECTION_LOD).xyz;
    float spec = pow(NdotH, SpecularRoughness);

    vec3 f0 = vec3(0.01);
    float roughness = 0.1;

    vec3 F = FresnelSchlickRoughness(HdotV, f0, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float D = GeometrySchlickGGX(NdotH, roughness);
    vec3 ks = F;
    vec3 kd = vec3(1.0) - ks;

    vec3 light = vec3(0);

    // Specular part
    vec3 nom = F*D*G;
    float denom = (4.0 * NdotV * NdotL + 1e-3);
    vec3 Specular = nom / denom;

    // Diffuse part
    vec3 SSS = kd * SSSColor * max(0.0, dot(V, -L)) * NdotV * NdotV * max(0.0, 0.5 - max(0.0, V.y)) / PI;
    vec3 Diffuse = kd * AmbWaterColor / PI;

    // Ambient
    ks = FresnelSchlickRoughness(NdotV, f0, 1.0);
    kd = vec3(1.0) - ks;
    vec3 ambient_diffuse = Irradiance * WaterColor;
    vec3 prefilteredColor = textureLod(Skybox, -R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdfOffset = texture(GGXLUT, vec2(NdotV, roughness)).xy;
    vec3 ambient_specular = prefilteredColor * (ks * brdfOffset.x + brdfOffset.y);
    vec3 Ambient = vec3(0);//kd * ambient_diffuse + ambient_specular;

    light = Ambient + ((Specular + Diffuse) * NdotL + SSS)  * LightColor.xyz;// + LightColor.xyz * NdotL * (Diffuse + Specular);


        //light += F*D*G * NdotL / (4.0 * NdotV * NdotL + 1e-4) * FCol;
        //light += fake_kd * WaterColor;
        //light += fake_ks * WaterCrestColor.xyz;
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
