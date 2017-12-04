#version 400

// TODO - give them as uniform from program constants, or create the shader from the code with this hardcoded
#define TRANSMITTANCE_TEXTURE_WIDTH 256
#define TRANSMITTANCE_TEXTURE_HEIGHT 64

#define IRRADIANCE_TEXTURE_WIDTH 64
#define IRRADIANCE_TEXTURE_HEIGHT 16

// TODO - put the structs and common functions in a shared header
struct density_profile_layer
{
    float Width;
    float ExpTerm;
    float ExpScale;
    float LinearTerm;
    float ConstantTerm;
};

struct density_profile
{
    density_profile_layer Layers[2];
};

struct atmosphere_parameters
{
    float           TopRadius;
    float           BottomRadius;
    vec3            RayleighScattering;
    density_profile RayleighDensity;
    vec3            MieExtinction;
    vec3            MieScattering;
    density_profile MieDensity;
    vec3            AbsorptionExtinction;
    density_profile AbsorptionDensity;
    vec3            GroundAlbedo;
    vec3            SolarIrradiance;
    float           SunAngularRadius;
    float           MiePhaseG;
};

uniform atmosphere_parameters Atmosphere;
uniform int ProgramUnit;
uniform sampler2D Tex0;
out vec4 out0;
out vec4 out1;

float ClampDistance(float D)
{
    return max(D, 0.0);
}

float ClampCosine(float C)
{
    return clamp(C, -1.0, 1.0);
}

float ClampRadius(float R)
{
   return clamp(R, Atmosphere.BottomRadius, Atmosphere.TopRadius); 
}

float SafeSqrt(float V)
{
    return sqrt(max(V, 0));
}

float DistanceToBottomBoundary(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + Atmosphere.BottomRadius * Atmosphere.BottomRadius;
    return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

float DistanceToTopBoundary(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + Atmosphere.TopRadius * Atmosphere.TopRadius;
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

// Mappings from x in [0,1] to texcoord u in [0.5/n, 1 - 0.5/n]
float GetTextureCoordFromUnitRange(float x, int TexSize)
{
    return 0.5 / float(TexSize) + x * (1.0 - 1.0 / float(TexSize));
}

float GetUnitRangeFromTextureCoord(float u, int TexSize)
{
    return (u - 0.5 / float(TexSize)) / (1.0 - 1.0 / float(TexSize));
}

vec2 GetTransmittanceTextureUVFromRMu(float r, float mu)
{
    float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float rho = SafeSqrt(r * r - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float d = DistanceToTopBoundary(r, mu);
    float d_min = Atmosphere.TopRadius - r;
    float d_max = rho + H;
    float x_mu = (d - d_min) / (d_max - d_min);
    float x_r = rho / H;
    return vec2(GetTextureCoordFromUnitRange(x_mu, TRANSMITTANCE_TEXTURE_WIDTH), GetTextureCoordFromUnitRange(x_r, TRANSMITTANCE_TEXTURE_HEIGHT));
}

void GetRMuFromTransmittanceTextureUV(vec2 uv, out float r, out float mu)
{
    float x_mu = GetUnitRangeFromTextureCoord(uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
    float x_r = GetUnitRangeFromTextureCoord(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);
    float H = sqrt(Atmosphere.TopRadius * Atmosphere.TopRadius - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float rho = SafeSqrt(x_r * x_r - Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    r = sqrt(rho * rho + Atmosphere.BottomRadius * Atmosphere.BottomRadius);
    float d_min = Atmosphere.TopRadius - r;
    float d_max = rho + H;
    float d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * r * d);
    mu = ClampCosine(mu);
}

vec2 GetIrradianceTextureUVFromRMuS(float r, float mu_s)
{
    float x_r = (r - Atmosphere.BottomRadius) / (Atmosphere.TopRadius - Atmosphere.BottomRadius);
    float x_mu_s = mu_s * 0.5 + 0.5;
    return vec2(GetTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH), GetTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
}

void GetRMuSFromIrradianceTextureUV(vec2 uv, out float r, out float mu_s)
{
    float x_mu_s = GetUnitRangeFromTextureCoord(uv.x, IRRADIANCE_TEXTURE_WIDTH);
    float x_r = GetUnitRangeFromTextureCoord(uv.y, IRRADIANCE_TEXTURE_HEIGHT);
    r = Atmosphere.BottomRadius + x_r * (Atmosphere.TopRadius - Atmosphere.BottomRadius);
    mu_s = ClampCosine(2.0 * x_mu_s - 1.0);
}

float GetLayerDensity(in density_profile_layer layer, float altitude)
{
    float density = layer.ExpTerm * exp(layer.ExpScale * altitude) + layer.LinearTerm * altitude + layer.ConstantTerm;
    return clamp(density, 0.0, 1.0);
}

float GetProfileDensity(in density_profile profile, float altitude)
{
    return altitude < profile.Layers[0].Width ? GetLayerDensity(profile.Layers[0], altitude)
                                              : GetLayerDensity(profile.Layers[1], altitude);
}

float ComputeOpticalLengthToTopAtmosphereBoundary(in density_profile profile, float r, float mu)
{
    int SAMPLE_COUNT = 500;
    float dx = DistanceToTopBoundary(r, mu) / float(SAMPLE_COUNT);
    float result = 0.0;
    for(int i = 0; i < SAMPLE_COUNT; ++i)
    {
        float d_i = float(i) * dx;
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        float y_i = GetProfileDensity(profile, r_i - Atmosphere.BottomRadius);
        float w_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
        result += y_i * w_i * dx;
    }
    return result;
}

vec3 ComputeTransmittanceToTopAtmosphereBoundary(float r, float mu)
{
    return exp(-(Atmosphere.RayleighScattering * ComputeOpticalLengthToTopAtmosphereBoundary(Atmosphere.RayleighDensity, r, mu)
                 + Atmosphere.MieExtinction * ComputeOpticalLengthToTopAtmosphereBoundary(Atmosphere.MieDensity, r, mu)
                 + Atmosphere.AbsorptionExtinction * ComputeOpticalLengthToTopAtmosphereBoundary(Atmosphere.AbsorptionDensity, r, mu)
                 ));
}
vec3 ComputeTransmittanceToTopAtmosphereBoundaryTexture(vec2 FragCoord)
{
    vec2 TexSize = vec2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    float r, mu;
    GetRMuFromTransmittanceTextureUV(FragCoord / TexSize, r, mu);
    return ComputeTransmittanceToTopAtmosphereBoundary(r, mu);
}

vec3 GetTransmittanceToTopAtmosphereBoundary(in sampler2D TransmittanceTexture, float r, float mu)
{
    vec2 uv = GetTransmittanceTextureUVFromRMu(r, mu);
    return texture(TransmittanceTexture, uv).xyz;
}

vec3 ComputeDirectIrradiance(float r, float mu_s)
{
    float alpha_s = Atmosphere.SunAngularRadius;
    float avg_cos_factor = mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s : (mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));

    return Atmosphere.SolarIrradiance * GetTransmittanceToTopAtmosphereBoundary(Tex0, r, mu_s) * avg_cos_factor;
}

vec3 ComputeDirectIrradianceTexture(vec2 FragCoord)
{
    vec2 TexSize = vec2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
    float r, mu_s;
    GetRMuSFromIrradianceTextureUV(FragCoord / TexSize, r, mu_s);
    return ComputeDirectIrradiance(r, mu_s);
}

void main()
{
    if(ProgramUnit == 0)
    { // Transmittance texture
        out0 = vec4(ComputeTransmittanceToTopAtmosphereBoundaryTexture(gl_FragCoord.xy), 1);
    }
    else if(ProgramUnit == 1)
    { // Ground Irradiance texture
        out0 = vec4(ComputeDirectIrradianceTexture(gl_FragCoord.xy), 1); // delta irradiance
        out1 = vec4(0.0); // irradiance (nothing yet, no direct)
    }
    else
    {
        out0 = vec4(1,0,1, 1);
    }
}
