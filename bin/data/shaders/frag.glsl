#version 400
#define PI 3.14159265359

in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_halfvector;
in vec3 v_sundirection;

uniform sampler2D Albedo;
uniform sampler2D MetallicRoughness; // x is metallic, y is roughness
uniform vec3  AlbedoMult;
uniform float MetallicMult;
uniform float RoughnessMult;

uniform samplerCube IrradianceCubemap;
uniform samplerCube Skybox;

uniform vec4 LightColor;
uniform vec3 CameraPos;


out vec4 frag_color;

vec3 FresnelSchlick(float u, vec3 f0, float f90)
{
    return f0 + (vec3(f90) - f0) * pow(1 - u, 5);
}

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
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

void main()
{
    vec3 N = normalize(v_normal);
    vec3 L = normalize(v_sundirection);
    vec3 V = normalize(CameraPos - v_position);
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);

    vec3 albedo = pow(texture(Albedo, v_texcoord).xyz, vec3(2.2)) * AlbedoMult;
    float metallic = texture(MetallicRoughness, v_texcoord).x * MetallicMult;
    float roughness = texture(MetallicRoughness, v_texcoord).y * RoughnessMult;
    vec3 env_light = pow(texture(Skybox, R).xyz, vec3(2.2));
    vec3 irr_light = pow(texture(IrradianceCubemap, -R).xyz, vec3(1.0));

    float NdotV = min(1, abs(dot(N, V)) + 1e-1);//1e-5f);
    float NdotL = max(0, dot(N, L));
    float HdotV = max(0, dot(H, V));
    float NdotH = max(0, dot(N, H));
    float LdotH = max(0, dot(L, H));

    // TODO - This is the baseline for a dielectric material
    // TODO - should be parameterized
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic); // if metallic, linearly interpolate towards metallic color

    // Specular part
    vec3 F = FresnelSchlick(LdotH, f0, 1);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float D = DistributionGGX(NdotH, roughness);

    vec3 nom = F * G * D;
    float denom = (4 * NdotV * NdotL + 1e-4);

    vec3 Specular = nom * NdotL / denom;
    vec3 Diffuse = (1 - metallic) * DisneyFrostbite(NdotV, NdotL, LdotH, 1 - roughness) * albedo * NdotL / PI;

    // Ambient
    float ks = FresnelSchlick(min(1,abs(dot(N,V))+1e-1), f0, roughness).x;
    float kd = 1.0 - ks;
    kd *= (1 - metallic);
    vec3 ambient_diffuse = irr_light * albedo;
    vec3 ambient_specular = vec3(0);
    vec3 Ambient = kd * ambient_diffuse + ambient_specular;

    vec3 color = Ambient + LightColor.xyz * (Diffuse + Specular);

    frag_color = vec4(color, 1.0);
}
