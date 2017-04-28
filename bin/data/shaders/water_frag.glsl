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

out vec4 frag_color;

#if 0
float FresnelF0(in float n1, in float n2) {
    float f0 = (n1 - n2)/(n1 + n2);
    return f0 * f0;
}

float FresnelRatio(in float u)
{
    return pow(1.0 - u, 5.0);
}

vec3 FresnelSchlick(in vec3 f0, in vec3 f90, in float u) {
    return f0 + (f90 - f0) * FresnelRatio(u);
}
#endif

vec3 FresnelSchlick(float HdotV, vec3 F0, vec3 F90)
{
    return F0 + (F90 - F0) * pow(1 - HdotV, 5);
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
    float water_dist = length(CameraPos - v_position);
    if(water_dist == 0) discard;

    vec3 N = normalize(v_normal);
    vec3 L = normalize(v_sundirection);
    vec3 V = (CameraPos - v_position) / water_dist;
    vec3 H = normalize(V + L);
    vec3 R = reflect(V, N);


    float NdotL = max(0.0, dot(N, L));
    float NdotH = max(0.0, dot(N, H));
    float NdotV = max(1e-3, dot(N, V));
    float LdotH = max(0.0, dot(L, H));
    float HdotV = max(0.0, dot(H, V));
    float VdotR = max(0.0, dot(V, R));
    float NdotInvL = max(0.0, dot(-L, N));
    float VdotInvL = max(0.0, dot(-L, V));
    float VdotInvL2 = pow(VdotInvL,5);

    // Parameters
    float nSnell = 1.34;
    float sigma_a = 0.00005;
    float sigma_s = 0.0000;
    float sigma_t = sigma_a + sigma_s;
    float reflect_ratio = 0.5;
    float refract_ratio = 0.5;
    vec3 fog_color = vec3(0.30, 0.40, 0.50);
    vec3 sky_color = vec3(0.20, 0.40, 0.65);
    vec3 water_color = vec3(0.01, 0.10, 0.31);
    //vec3 water_color = vec3(0.001, 0.012, 0.035);
    vec3 sss_color = vec3(0.00, 0.95, 0.9);
    vec3 specular_color = vec3(1,1,1);
    vec3 env_light = pow(texture(Skybox, -R).xyz, vec3(2.2));
    vec3 irr_light = pow(texture(IrradianceCubemap, -R).xyz, vec3(2.2));


    // TODO - Replace this with specular IBL
    vec3 reflection = (1 - reflect_ratio) * vec3(1) + reflect_ratio * env_light;

    vec3 color = vec3(0);

#if 0
    if(gl_FrontFacing)
    {
#endif
        float roughness = 0.001;
        float dist = exp(-sigma_t * water_dist);

        vec3 WaterColor = mix(irr_light, sss_color, max(0, 0.5-max(0,dot(V,vec3(0,1,0)))) * VdotInvL2 * NdotV);
        vec3 F = FresnelSchlick(NdotV, water_color, reflection*sky_color / nSnell);
        //float G = GeometrySmith(NdotV, NdotL, roughness);
        //float D = GeometrySchlickGGX(NdotH, roughness);

        color = (1 - dist) * fog_color;
        color += dist * (F);
        // TODO - Specular reflection on normal map
#if 0
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

    // Gamma correction
    color = color / (color + vec3(1));
    color = pow(color, vec3(1.0/2.2));

    frag_color = vec4(color, 1);
}

