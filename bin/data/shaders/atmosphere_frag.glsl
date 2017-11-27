#version 400

struct atmosphere_parameters
{
    float TopRadius;
    float BottomRadius;
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

float SafeSqrt(float V)
{
    return sqrt(max(V, 0));
}

float DistanceToTopBoundary(in atmosphere_parameters atmosphere, float r, float mu)
{
    //assert(r <= atmosphere.TopRadius);
    //assert(mu >= -1.0 && mu <= 1.0);
    float discriminant = r * r * (mu * mu - 1.0) + atmosphere.TopRadius * atmosphere.TopRadius;
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

void main()
{
    vec3 E = normalize(v_eyeRay);
    float r = length(CameraPosition);
    float rmu = dot(CameraPosition, E);
    float mu = rmu / r;
    float D = DistanceToTopBoundary(Atmosphere, r, mu);
    frag_color = vec4(0,0, abs(D)/100000000, 1); 
}
