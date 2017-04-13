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

vec3 FresnelSchlick(in vec3 f0, in float f90, in float u) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

vec4 GGX(in float NdotL, in float NdotV, in float NdotH, in float LdotH, in float roughness, in vec3 F0) {
    float alpha2 = roughness * roughness;

    // F 
    vec3 F = FresnelSchlick(F0, 1.0, LdotH);

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
    float fd_90 = energy_bias + 2.0 * LdotH * LdotH * roughness;
    vec3 f0 = vec3(1.0);
    float light_scatter = FresnelSchlick(f0, fd_90, NdotL).r;
    float view_scatter = FresnelSchlick(f0, fd_90, NdotV).r;

    return light_scatter * view_scatter * energy_factor * M_INV_PI;
}

void main()
{
    vec3 N = normalize(v_normal);
    vec3 L = normalize(v_sundirection);
    float dCP = length(CameraPos - v_position);
    if(dCP == 0) discard;
    vec3 V = (CameraPos - v_position) / dCP;
    vec3 H = normalize(V + L);


    vec3 reflect_vec = reflect(V, N);
    vec4 reflect_color = texture(Skybox, reflect_vec);

    float NdotL = max(0.0, dot(N, L));

    vec4 lighting = vec4(0, 0, 0, 0);

    float fog_range = 0.001;
    vec4 air = vec4(0.80, 0.85, 1.0, 1.0) * (0.75 + 0.25 * reflect_color);
    vec4 sky = vec4(0.19, 0.34, 0.5, 1.0);
    vec4 upwelling = vec4(0.0, 0.2, 0.3, 1.0);
    float kDiffuse = 0.91f;
    float nSnell = 1.34f;

    float dist = fog_range * dCP * kDiffuse;
    dist = exp(-dist);

    float reflectivity = 0.0;

    float costheta_V = NdotL;
    float theta_V = acos(NdotL);
    float sintheta_T = sin(theta_V) / nSnell;
    float theta_T = asin(sintheta_T);
    if(theta_V == 0.0)
    {
        reflectivity = (nSnell - 1) / (nSnell + 1);
        reflectivity = reflectivity * reflectivity;
    }
    else
    {
        float fs = sin(theta_T - theta_V) / sin(theta_T + theta_V);
        float ts = tan(theta_T - theta_V) / tan(theta_T + theta_V);
        reflectivity = 0.5 * (fs * fs + ts * ts);
    }

    lighting = (1 - dist) * air +
                dist * (reflectivity * sky + 
                        (1 - reflectivity) * upwelling);

    frag_color = lighting;
    frag_color.a = 1.0;
    
#if 0
    vec4 emissive_color = vec4(0.0, 1.0, 0.5,  1.0);
	vec4 ambient_color  = vec4(0.0, 0.35, 0.45, 1.0);
	vec4 diffuse_color  = vec4(0.1, 0.45, 0.55, 1.0);
	vec4 specular_color = vec4(0.4, 0.4, 0.4,  1.0);

	float emissive_contribution = 0.35;
	float ambient_contribution  = 0.20;
	float diffuse_contribution  = 1.00;
	float specular_contribution = 0.10;

    float roughness = 0.01;

    if(NdotL > 0.0)
    {
        float NdotH = max(0.0, dot(N, H));
        float NdotV = max(0.0, dot(N, V));
        float LdotH = max(0.0, dot(L, H));

        vec4 Fd = diffuse_color * diffuse_contribution * DiffuseBurley(NdotL, NdotV, LdotH, roughness);
        //vec4 Fr = specular_color * specular_contribution * pow(max(0.0, NdotH), 1.0/(roughness*roughness));
        //vec4 Fr = GGX(NdotL, NdotV, NdotH, LdotH, roughness, specular_contribution * specular_color.xyz);
        vec4 Fr = vec4(0,0,0,0);//reflect_color;

        //lighting += LightColor * (Fd + Fr) * NdotL;
    }

    //frag_color = emissive_color * emissive_contribution * reflect_color * (1- max(0.0, min(1.0, 0.75+d))) +
                 //ambient_color * ambient_contribution * reflect_color +
                 //diffuse_color * diffuse_contribution * reflect_color * max(0.0, d);// +
                 //(facing ? specular_color * specular_contribution * reflect_color * max(0.0, pow(dot(H, N), 120.0)):
                 //vec4(0.0, 0.0, 0.0, 0.0));
#endif
}

