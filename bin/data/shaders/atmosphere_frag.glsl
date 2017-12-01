#version 400
#define PI 3.14159265359

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
    density_profile MieDensity;
    vec3            AbsorptionExtinction;
    density_profile AbsorptionDensity;
    vec3            GroundAlbedo;
    vec3            SolarIrradiance;
    float           SunAngularRadius;
    float           MiePhaseG;
};


in vec2 v_texcoord;
in vec3 v_eyeRay;

uniform atmosphere_parameters Atmosphere;
uniform vec3 CameraPosition;
uniform vec3 SunDirection;

out vec4 frag_color;

float ClampDistance(float D)
{
    return max(D, 0.0);
}

float ClampCosine(float C)
{
    return clamp(C, -1.0, 1.0);
}

float ClampRadius(in atmosphere_parameters atmosphere, float R)
{
   return clamp(R, atmosphere.BottomRadius, atmosphere.TopRadius); 
}

float SafeSqrt(float V)
{
    return sqrt(max(V, 0));
}

float DistanceToBottomBoundary(in atmosphere_parameters atmosphere, float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + atmosphere.BottomRadius * atmosphere.BottomRadius;
    return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

float DistanceToTopBoundary(in atmosphere_parameters atmosphere, float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + atmosphere.TopRadius * atmosphere.TopRadius;
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

bool RayIntersectsGround(in atmosphere_parameters atmosphere, float r, float mu)
{
    return mu < 0.0 && (r * r * (mu * mu - 1.0) + atmosphere.BottomRadius * atmosphere.BottomRadius) >= 0.0;
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

float ComputeOpticalLengthToTopAtmosphereBoundary(in atmosphere_parameters atmosphere, in density_profile profile, float
r, float mu)
{
    int SAMPLE_COUNT = 10;
    float dx = DistanceToTopBoundary(atmosphere, r, mu) / float(SAMPLE_COUNT);
    float result = 0.0;
    for(int i = 0; i < SAMPLE_COUNT; ++i)
    {
        float d_i = float(i) * dx;
        float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        float y_i = GetProfileDensity(profile, r_i - atmosphere.BottomRadius);
        float w_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0;
        result += y_i * w_i * dx;
    }
    return result;
}

vec3 ComputeTransmittanceToTopAtmosphereBoundary(in atmosphere_parameters atmosphere, float r, float mu)
{
    return exp(-(atmosphere.RayleighScattering * ComputeOpticalLengthToTopAtmosphereBoundary(atmosphere, atmosphere.RayleighDensity, r, mu)
                 + atmosphere.MieExtinction * ComputeOpticalLengthToTopAtmosphereBoundary(atmosphere, atmosphere.MieDensity, r, mu)
                 + atmosphere.AbsorptionExtinction * ComputeOpticalLengthToTopAtmosphereBoundary(atmosphere, atmosphere.AbsorptionDensity, r, mu)
                 ));
}

float RayleighPhaseFunction(float nu)
{
    float k = 3.0 / (16.0 * PI);
    return k * (1.0 + nu * nu);
}

float MiePhaseFunction(float g, float nu)
{
    float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

vec3 ComputeCombinedScattering(in atmosphere_parameters atmosphere, float r, float mu, float mu_s, float nu, out vec3 Mie)
{
    float rmu = r * mu;
    float d = -rmu - sqrt(rmu * rmu - r * r + atmosphere.TopRadius * atmosphere.TopRadius);
    float r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu * d) / r_d);

    Mie = vec3(0);
    vec3 contrib = min(ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu) /
            ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r_d, mu_d), vec3(1));
    return contrib;
}

vec3 GetSkyRadiance(in atmosphere_parameters atmosphere, vec3 P, vec3 E, out vec3 Transmittance)
{
    vec3 L = normalize(SunDirection);
    float r = length(P);
    float rmu = dot(P, E);

    float d = -rmu - sqrt(rmu * rmu - r * r + atmosphere.TopRadius * atmosphere.TopRadius);
    if(d > 0.0)
    { // if outside atmosphere looking in, move along the ray to the top boundary
        P = P + E * d;
        r = atmosphere.TopRadius;
        rmu += d;
    }
    else if(r > atmosphere.TopRadius)
    { // If we are outside the atmosphere and not looking in, return 0 (space)
        return vec3(0);
    }

    vec3 contrib = vec3(0);
    float mu = rmu / r;
    float mu_s = dot(P, L) / r;
    float nu = max(0.0,dot(E, L));

    float r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
    float mu_d = ClampCosine((r * mu * d) / r_d);

    Transmittance = min(vec3(1), ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu));
    //contrib += min(ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu) /
            //ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r_d, mu_d), vec3(1));
    vec3 MieScattering;
    contrib += ComputeCombinedScattering(atmosphere, r, mu, mu_s, nu, MieScattering);

    return contrib * RayleighPhaseFunction(nu) + MieScattering * MiePhaseFunction(atmosphere.MiePhaseG, nu);
}

vec3 GetSkyRadianceToPoint(in atmosphere_parameters atmosphere, vec3 Camera, vec3 P, vec3 L)
{
    return vec3(0);
}

vec3 GetTransmittanceToSun(in atmosphere_parameters atmosphere, float r, float mu_s)
{
    float SinThetaH = atmosphere.BottomRadius / r;
    float CosThetaH = -sqrt(max(1.0 - SinThetaH * SinThetaH, 0.0));
    return vec3(ComputeTransmittanceToTopAtmosphereBoundary(atmosphere, r, mu_s)) *
            smoothstep(-SinThetaH * atmosphere.SunAngularRadius,
                       SinThetaH * atmosphere.SunAngularRadius,
                       mu_s * CosThetaH);
}

vec3 GetGroundIrradiance(in atmosphere_parameters atmosphere, vec3 P, vec3 N, vec3 L)
{
    float r = length(P);
    float mu_s = dot(P, L) / r;

    return atmosphere.SolarIrradiance * max(0.0, dot(N, L)) * GetTransmittanceToSun(atmosphere, r, mu_s);
}

vec3 GetSolarRadiance()
{
    return Atmosphere.SolarIrradiance / (PI * Atmosphere.SunAngularRadius * Atmosphere.SunAngularRadius);
}

void main()
{
    vec3 E = normalize(v_eyeRay);
    vec3 EarthCenter = vec3(0,-Atmosphere.BottomRadius, 0);
    vec3 p = CameraPosition - EarthCenter;
    float PdotV = dot(p, E);
    float PdotP = dot(p, p);
    float EarthCenterRayDistance = PdotP - PdotV * PdotV;
    float Depth = -PdotV - sqrt(Atmosphere.BottomRadius*Atmosphere.BottomRadius - EarthCenterRayDistance);

    vec3 L = normalize(SunDirection);

    vec3 contrib = vec3(0);
    if(Depth > 0.0)
    {
        vec3 Point = CameraPosition + E * Depth;
        vec3 N = normalize(Point - EarthCenter);
        Point += N * 1e-3; // To avoid depth fighting

        vec3 SunRadiance = GetGroundIrradiance(Atmosphere, Point - EarthCenter, N, L);
        contrib += 1e-4 * SunRadiance * Atmosphere.GroundAlbedo * (1.0/PI);
    }
    else
    {
        vec3 Transmittance;
        contrib += max(vec3(0), GetSkyRadiance(Atmosphere, p, E, Transmittance));
        if(dot(E, L) > cos(Atmosphere.SunAngularRadius))
        {
            contrib += Transmittance * GetSolarRadiance();
        }
    }

    frag_color = vec4(contrib, 1); 
}
