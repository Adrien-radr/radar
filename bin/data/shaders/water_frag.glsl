#version 400

#define M_PI     3.14159265358
#define M_INV_PI 0.31830988618

in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_sundirection;

uniform samplerCube Skybox;
uniform vec4 LightColor;
uniform vec3 CameraPos;

out vec4 frag_color;

float FresnelF0(in float n1, in float n2) {
    float f0 = (n1 - n2)/(n1 + n2);
    return f0 * f0;
}

vec3 FresnelSchlick(in vec3 f0, in vec3 f90, in float u) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

vec4 GGX(in float NdotL, in float NdotV, in float NdotH, in float LdotH, in float roughness, in vec3 F0) {
    float alpha2 = roughness * roughness;

    // F 
    vec3 F = FresnelSchlick(F0, vec3(1.0), LdotH);

    // D (Trowbridge-Reitz). Divide by PI at the end
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (M_PI * denom * denom);

    // G (Smith GGX - Height-correlated)
    float lambda_GGXV = NdotL * sqrt((-NdotV * alpha2 + NdotV) * NdotV + alpha2);
    float lambda_GGXL = NdotV * sqrt((-NdotL * alpha2 + NdotL) * NdotL + alpha2);
    // float G = G_schlick_GGX(k, NdotL) * G_schlick_GGX(k, NdotV);

    // optimized G(NdotL) * G(NdotV) / (4 NdotV NdotL)
    // float G = 1.0 / (4.0 * (NdotL * (1 - k) + k) * (NdotV * (1 - k) + k));
    float G = 0.5 / (lambda_GGXL + lambda_GGXV);

    return vec4(D * F * G, 1.0);
}

float DiffuseLambert(in float NdotL) {
    return M_INV_PI;
}

float DiffuseBurley(in float NdotL, in float NdotV, in float LdotH, in float roughness) {
    float energy_bias = mix(0.0, 0.5, roughness);
    float energy_factor = mix(1.0, 1.0 / 1.51, roughness);
    vec3 fd_90 = vec3(energy_bias + 2.0 * LdotH * LdotH * roughness);
    vec3 f0 = vec3(1.0);
    float light_scatter = FresnelSchlick(f0, fd_90, NdotL).r;
    float view_scatter = FresnelSchlick(f0, fd_90, NdotV).r;

    return light_scatter * view_scatter * energy_factor * M_INV_PI;
}

void main()
{
    float water_dist = length(CameraPos - v_position);
    if(water_dist == 0) discard;

    vec3 N = normalize(v_normal);
    vec3 L = normalize(v_sundirection);
    vec3 V = (CameraPos - v_position) / water_dist;
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);


    float NdotL = max(0.0, dot(N, L));
    float NdotH = max(0.0, dot(N, H));
    float NdotV = max(0.0, dot(N, V));
    float LdotH = max(0.0, dot(L, H));
    float VdotR = max(0.0, dot(V, R));
    float NdotInvL = max(0.0, dot(-L, N));
    float VdotInvL = max(0.0, dot(-L, V));
    float VdotInvL2 = pow(VdotInvL,2);

    // Parameters
    float nSnell = 1.34;
    float sigma_a = 0.0008;
    float sigma_s = 0.0000;
    float sigma_t = sigma_a + sigma_s;
    float reflect_ratio = 0.25;
    vec4 fog_color = vec4(0.30, 0.40, 0.50, 1.0);
    vec4 sky_color = vec4(0.35, 0.50, 0.65, 1.0);
    vec4 water_color = vec4(0.01, 0.19, 0.31, 1.0);
    vec4 sss_color = vec4(0.4, 0.9, 0.05, 1.0);
    vec4 specular_color = vec4(1,1,1,1);
    vec4 reflect_color = texture(Skybox, R);

    // Underwater params
    float sigma_a_uw = 0.05;
    float sigma_s_uw = 0.00;
    float sigma_t_uw = sigma_a_uw + sigma_s_uw;
    vec4 uw_far_color = vec4(0.0075, 0.15, 0.23, 1.0);
    vec4 uw_close_color = vec4(0.4, 0.8, 0.9, 1.0);

    vec4 reflection = (1 - reflect_ratio) * vec4(1) + reflect_ratio * reflect_color;
    vec3 Fresnel = FresnelSchlick(water_color.xyz, reflection.xyz * sky_color.xyz/nSnell, NdotV);

    if(gl_FrontFacing)
    {
        float dist = exp(-sigma_t * water_dist);
        float Specular = NdotL * pow(VdotR, 240.0); 
        float SSS = max(0, 0.5-max(0,dot(V,vec3(0,1,0)))) * NdotV;

        frag_color = (1 - dist) * reflection * fog_color;                           // Fog
        frag_color += dist * (vec4(Fresnel, 1.0) +                                  // Fresnel term
                VdotInvL2 * (SSS * sss_color +                        // Subsurface Scattering Hack
                    Specular * reflection * specular_color));// Specular reflection
        frag_color.a = 1.0;
    }
    else
    {
        float uwNdotV = max(0, dot(V, -N));
        float uwVdotInvL2 = pow(max(0, dot(L, -V)), 2);

        float dist = exp(-sigma_t_uw * water_dist);
        float SSS = uwVdotInvL2 * uwNdotV;//max(0, 0.5-max(0,dot(V,vec3(0,-1,0)))) * uwNdotV;

        frag_color = (1-dist) * uw_far_color;
        frag_color += dist * ((1-uwNdotV) * uw_far_color + uwNdotV * uw_close_color);
        frag_color.a = 0.9 + 0.1 * (1-dist);
    }
}

