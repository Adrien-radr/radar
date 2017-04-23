#version 400
#define PI 3.14159265359

in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_halfvector;
in vec3 v_sundirection;

uniform sampler2D DiffuseTexture;
uniform samplerCube Skybox;
uniform vec4 LightColor;
uniform vec3 CameraPos;

uniform vec3  Albedo;
uniform float Metallic;
uniform float Roughness;

out vec4 frag_color;

vec3 FresnelSchlick(float HdotV, vec3 F0)
{
    return F0 + (1 - F0) * pow(1 - HdotV, 5);
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

void main()
{
    vec4 diffuse_texture = texture(DiffuseTexture, v_texcoord);

    vec3 N = normalize(v_normal);
    vec3 L = normalize(v_sundirection);
    vec3 V = normalize(CameraPos - v_position);
    vec3 H = normalize(V + L);

    float NdotV = max(0, dot(N, V));
    float NdotL = max(0, dot(N, L));
    float HdotV = max(0, dot(H, V));
    float NdotH = max(0, dot(N, H));

    vec3 albedo = Albedo * diffuse_texture.xyz;

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, Metallic);

    // Specular part
    vec3 F = FresnelSchlick(HdotV, f0);
    float G = GeometrySmith(NdotV, NdotL, Roughness);
    float D = DistributionGGX(NdotH, Roughness);

    vec3 nom = F * G * D;
    float denom = (4 * NdotV * NdotL + 0.001);

    vec3 Specular = nom / denom;

    // Diffuse part
    vec3 ks = F;
    vec3 kd = vec3(1) - ks;
    kd *= 1 - Metallic;

    vec3 Diffuse = kd * albedo / PI;
    vec3 Ambient = vec3(0.03) * albedo;

    vec3 color = Ambient + (Diffuse + Specular) * NdotL * LightColor.xyz;

    // Gamma correction
    color = color / (color + vec3(1));
    color = pow(color, vec3(1.0/2.2));

    frag_color = vec4(color, 1.0);
}
