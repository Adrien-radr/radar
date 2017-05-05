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
const float SigmaA = 0.000015;
const float SigmaS = 0.0000;
const float SigmaT = SigmaA + SigmaS;
const vec3 FogColor = vec3(0.40, 0.50, 0.60);
const vec3 WaterColor = vec3(0.05, 0.20, 0.80);
const vec3 SSSColor = vec3(0, 0.8, 0.5);
const float SeaHeight = -10.0;
const float DiffusePart = 0.05;
const float SpecularPart = 4.0;
const float SpecularRoughness = 512;

#if 0
float FresnelF0(in float n1, in float n2) {
    float f0 = (n1 - n2)/(n1 + n2);
    return f0 * f0;
}


vec3 FresnelSchlick(in vec3 f0, in vec3 f90, in float u) {
    return f0 + (f90 - f0) * FresnelRatio(u);
}
#endif

float FresnelRatio(in float u)
{
    return pow(1.0 - u, 5.0);
}

vec3 FresnelColor(float Ratio, vec3 F0, vec3 F90)
{
    return F0 + (F90 - F0) * Ratio;
}

vec3 FresnelSchlickColor(float FRatio, vec3 F0, vec3 F90)
{
    return F0 + (F90 - F0) * FRatio;
}

vec3 FresnelSchlick(float FRatio, vec3 F0)
{
    return F0 + (1.0 - F0) * FRatio;
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

vec3 Sky(in vec3 Rd, in vec3 L, in vec3 light_rgb)
{
    float amount = max(dot(Rd, L), 0.0);
    float v = pow(1.0 - max(Rd.y, 0.0), 6.0);
    vec3 sky = mix(vec3(.1, .2, .4), vec3(.32, .32, .32), v);
    sky += light_rgb * amount * amount * .25 + light_rgb * min(pow(amount, 800.0)*1.5, 0.3);
    return sky;
}

float Visibility()
{
    return 1;
}

vec3 Shading(vec3 Pos, vec3 Rd, vec3 N, vec3 L, vec3 light_rgb)
{
    vec3 V = -Rd;
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);
    float NdotL = max(0.0, dot(N, L));
    float NdotV = max(1e-3, dot(N, V));
    float NdotH = max(0.0, dot(N, H));

    vec3 Irradiance = pow(texture(IrradianceCubemap, -R).xyz, vec3(2.2));
    float spec = pow(NdotH, SpecularRoughness);

    float F = FresnelRatio(dot(N, V));
    //F = mix(0, 1, min(1, F));

    vec3 FCol = FresnelSchlick(F, WaterColor * 0.02);
    float G = GeometrySmith(NdotV, NdotL, 0.02);
    float D = GeometrySchlickGGX(NdotH, 0.02);

    vec3 light = vec3(0);

    //float diffuse = DiffusePart;
    //float specular = SpecularPart * spec * (1.0 - F);
    //vec3 light = light_rgb * (diffuse + specular);

    // Diffuse
    float ks = F;
    float kd = 1 - ks;

    if(F > 0.0)
    {
        //vec3 env_light = mix(, pow(texture(Skybox, -R).xyz, vec3(2.2)), 0.02);
        light += F*G*D * NdotL / (4.0 * NdotV * NdotL + 1e-4) * Irradiance;
        light += spec * SpecularPart * kd * light_rgb;
        //vec3 WaterColor = mix(irr_light, sss_color, max(0, 0.5-max(0,dot(V,vec3(0,1,0)))) * VdotInvL2 * NdotV);
        //light += kd * WaterColor * DiffusePart * env_light * INV_PI;

        // fake SSS
        light += kd * SSSColor * NdotV * NdotV * pow(max(0.0, dot(V, -L)), 5) * max(0.0, 0.5 - max(0.0, dot(V, vec3(0, 1, 0))));
    }

    //light += (1-NdotL) * WaterColor * 0.02;

    //vec3 dist = Pos - CameraPos;
    //float atten = max(1.0 - dot(dist, dist) * 0.0001, 0.0);
    //light += WaterColor * (Pos.y - SeaHeight) * 0.10 * atten;
    //light *= vec3(0.05,0.2,0.7); 
    return light;
}

vec3 Fog(vec3 rgb, vec3 Rd, vec3 L, vec3 light_rgb, float dist, float sigma_t)
{
    float fog_amount = 1.0 - exp(-dist * sigma_t);
    return mix(rgb, light_rgb, fog_amount);
}

const vec2 FractDelta = vec2(0.01, 0.0);
const mat2 OctaveMat = mat2(1.6, 1.2, -1.2, 1.6);
const vec2 WindDirection = vec2(-0.2,  -2.2);
const vec2 WindStrength = vec2(0.11, 0.11);
const vec2 WaveInvScale = vec2(1.25, 0.25);

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
    float m = 1.5;
    float w = 1.0;
    float f = 0.0;
    for(int i = 0; i < 6; ++i)
    {
        //f += Noise(WaveInvScale.x * xy + Time * WindDirection.x) * m * WindStrength.x;
        f += Noise(WaveInvScale.y * xy + Time * WindDirection.y) * w * WindStrength.y;
        //f += Noise(xy.yx - Time * WindDirection.y) * w * WindStrength.y;
        w *= 0.5;
        m *= 0.25;
        xy *= OctaveMat;
    }
    return f;
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

    vec3 N = normalize(v_normal);
    vec3 FractN = GetFractalNormal(v_position, N);
    vec3 L = normalize(v_sundirection);
    //vec3 R = reflect(V, N);

    vec3 shading = Shading(v_position, Rd, FractN, L, LightColor.xyz);
    vec3 color = Fog(shading, Rd, L, LightColor.xyz, water_dist, SigmaT);

    // Gamma correction
    color = color / (color + vec3(1));
    color = pow(color, vec3(1.0/2.2));

    frag_color = vec4(color, 1);
}

#if 0
// BACKUP
    float NdotL = max(0.0, dot(N, L));
    float NdotH = max(0.0, dot(N, H));
    float NdotV = max(1e-3, dot(N, V));
    float LdotH = max(0.0, dot(L, H));
    float HdotV = max(0.0, dot(H, V));
    float VdotR = max(0.0, dot(V, R));
    float NdotInvL = max(0.0, dot(-L, N));
    float VdotInvL = max(0.0, dot(-L, V));
    float VdotInvL2 = pow(VdotInvL,5);

    float roughness = 0.001;
    float nSnell = 1.34;
    float reflect_ratio = 0.5;
    float refract_ratio = 0.5;
    vec3 sky_color = vec3(0.20, 0.40, 0.65);
    vec3 water_color = vec3(0.01, 0.10, 0.31);
    //vec3 water_color = vec3(0.001, 0.012, 0.035);
    vec3 sss_color = vec3(0.00, 0.95, 0.9);
    vec3 specular_color = vec3(1,1,1);
    vec3 env_light = pow(texture(Skybox, -R).xyz, vec3(2.2));
    vec3 irr_light = pow(texture(IrradianceCubemap, -R).xyz, vec3(2.2));


    // TODO - Replace this with specular IBL
    vec3 reflection = (1 - reflect_ratio) * vec3(1) + reflect_ratio * env_light;

    if(gl_FrontFacing)
    {
        //vec3 WaterColor = mix(irr_light, sss_color, max(0, 0.5-max(0,dot(V,vec3(0,1,0)))) * VdotInvL2 * NdotV);
        //vec3 F = FresnelSchlick(NdotV, water_color, reflection*sky_color / nSnell);
        //float G = GeometrySmith(NdotV, NdotL, roughness);
        //float D = GeometrySchlickGGX(NdotH, roughness);
        //color = (1 - dist) * fog_color;
        //color += dist * (F);
    }
    else
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
