#version 400
#define PI 3.14159265359
#define MAX_REFLECTION_LOD 4.0

in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_halfvector;
in vec3 v_sundirection;
in vec3 v_tangent;
in vec3 v_bitangent;

uniform vec3  AlbedoMult;
uniform vec3  EmissiveMult;
uniform float MetallicMult;
uniform float RoughnessMult;

// In Order
uniform sampler2D Albedo;
uniform sampler2D NormalMap;
uniform sampler2D MetallicRoughness; // x is metallic, y is roughness
uniform sampler2D Emissive;
uniform sampler2D GGXLUT;
uniform samplerCube Skybox;

uniform vec4 LightColor;
uniform vec3 CameraPos;


out vec4 frag_color;

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
    float lightScatter = FresnelSchlickRoughness(NdotL, f0, f90).r;
    float viewScatter = FresnelSchlickRoughness(NdotV, f0, f90).r;
    return lightScatter * viewScatter * eFactor;
}

void main()
{
    frag_color = vec4(texture(Albedo, v_texcoord).xyz, 1.0);
    #if 0
    vec3 localNormal = texture(NormalMap, v_texcoord).xyz;
    vec3 tN = normalize(2.0 * localNormal - vec3(1.0));
    vec3 N = normalize(v_normal);
    vec3 T = normalize(v_tangent);
    vec3 BT = normalize(v_bitangent);
    //vec3 T, BT;
    //BasisFrisvad(N, T, BT);
    mat3 TBN = mat3(T, BT, N);
    N = normalize(TBN * tN);
    vec3 L = normalize(v_sundirection);
    vec3 V = normalize(CameraPos - v_position);
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);

    vec3 emissive = pow(texture(Emissive, v_texcoord).xyz, vec3(2.2)) * EmissiveMult;
    vec3 albedo = pow(texture(Albedo, v_texcoord).xyz, vec3(2.2)) * AlbedoMult;
    vec2 MR = texture(MetallicRoughness, v_texcoord).xy;
    float metallic = min(1.0, MR.x * MetallicMult);
    float roughness = min(1.0, MR.y * RoughnessMult);

    //float NdotV = min(1, abs(dot(N, V)) + 1e-1);
    float NdotV = max(0, min(1,dot(N, V)));
    float NdotL = max(0, min(1,dot(N, L)));
    float HdotV = max(0, min(1,dot(H, V)));
    float NdotH = max(0, dot(N, H));
    float LdotH = max(0, dot(L, H));

    // TODO - This is the baseline for a dielectric material
    // TODO - should be parameterized
    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic); // if metallic, linearly interpolate towards metallic color

    // Specular part
    vec3 F = FresnelSchlick(HdotV, f0);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float D = DistributionGGX(NdotH, roughness);

    vec3 nom = F * G * D;
    float denom = (4 * NdotV * NdotL + 1e-3);

    vec3 Specular = nom / denom;

    // Diffuse part
    vec3 ks = F;
    vec3 kd = vec3(1.0) - ks;
    kd *= (1 - metallic);
    vec3 Diffuse = kd * albedo / PI;// * DisneyFrostbite(NdotV, NdotL, LdotH, 1 - roughness);

    // Ambient
    ks = FresnelSchlickRoughness(NdotV, f0, roughness);
    kd = vec3(1.0) - ks;
    kd *= (1 - metallic);
    vec3 irr_light = textureLod(Skybox, N, MAX_REFLECTION_LOD).xyz;
    vec3 ambient_diffuse = irr_light * albedo;

    vec3 prefilteredColor = textureLod(Skybox, -R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdfOffset = texture(GGXLUT, vec2(NdotV, roughness)).rg;
    vec3 ambient_specular = prefilteredColor * (ks * brdfOffset.x + brdfOffset.y);

    vec3 Ambient = kd * ambient_diffuse + ambient_specular;

    vec3 color = emissive + Ambient + LightColor.xyz * NdotL * (Diffuse + Specular);

    frag_color = vec4(color, 1.0);
    frag_color = vec4(MR,0, 1.0);
    #endif
}
