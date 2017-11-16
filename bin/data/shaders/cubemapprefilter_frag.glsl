#version 400
#define PI 3.14159265359

in vec3 v_texcoord;

uniform samplerCube Cubemap;
uniform float Roughness;

out vec4 frag_color;

// Van der Corpus sequence
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
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

vec3 GGX_IS(vec2 xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main()
{
    vec3 N = normalize(v_texcoord);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;

    vec3 prefilterColor = vec3(0);
    float totalWeight = 0.0;

    float saTexel = 4.0 * PI / (6.0 * 512 * 512); // 512 is the source cubemap size

    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = GGX_IS(xi, N, Roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            // Sample source Envmap mipmap lvl based on roughness
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float D = DistributionGGX(NdotH, Roughness);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 1e-4;
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 1e-4);
            float mipLevel = Roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilterColor += textureLod(Cubemap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilterColor = prefilterColor / totalWeight;

    frag_color = vec4(prefilterColor, 1);
}
