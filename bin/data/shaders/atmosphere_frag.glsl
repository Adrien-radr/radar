#version 400

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
};


in vec2 v_texcoord;
in vec3 v_eyeRay;

uniform atmosphere_parameters Atmosphere;
uniform vec3 CameraPosition;

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

void main()
{
    vec3 E = normalize(v_eyeRay);
    vec3 p = CameraPosition + vec3(0,0,Atmosphere.BottomRadius);
    float PdotV = dot(p, E);
    float PdotP = dot(p, p);
    float ray_earth_center_squared_distance = PdotP - PdotV * PdotV;
    float distance_to_intersection = -PdotV - sqrt(-ray_earth_center_squared_distance);
    float r = length(CameraPosition) + Atmosphere.BottomRadius;
    float mu = E.z;
    float rmu = mu * r;


    vec3 contrib = vec3(0);
    if(distance_to_intersection > 0.0)
    {
        contrib += vec3(1,0,0);
    }
    else
    {
        contrib += vec3(0,0,(PdotP-PdotV*PdotV)*1e-8);
    }
#if 0
    bool IsGround = RayIntersectsGround(Atmosphere, r, mu);
    if(IsGround)
    {
        float d = DistanceToBottomBoundary(Atmosphere, r, mu);
        float r_d = ClampRadius(Atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
        float mu_d = ClampCosine((r * mu * d) / r_d);
        contrib += min(ComputeTransmittanceToTopAtmosphereBoundary(Atmosphere, r_d, -mu_d) /
                       ComputeTransmittanceToTopAtmosphereBoundary(Atmosphere, r, -mu), vec3(1));
    }
    else
    {
        float d = DistanceToTopBoundary(Atmosphere, r, mu);
        float r_d = ClampRadius(Atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
        float mu_d = ClampCosine((r * mu * d) / r_d);
        contrib += min(ComputeTransmittanceToTopAtmosphereBoundary(Atmosphere, r, mu) /
                       ComputeTransmittanceToTopAtmosphereBoundary(Atmosphere, r_d, mu_d), vec3(1));
    }
    #endif

    frag_color = vec4(contrib, 1); 
}
