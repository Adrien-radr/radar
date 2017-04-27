#version 400
#define PI 3.14159265359

in vec3 v_texcoord;

uniform samplerCube Cubemap;

out vec4 frag_color;

mat3 BasisFrisvald(vec3 N)
{
    vec3 R;
    vec3 U;
    if(N.z < -0.9999999)
    {
        R = vec3(0, -1, 0);
        U = vec3(-1, 0, 0);
    }
    else
    {
        float a = 1.0 / (1.0 + N.z);
        float b = -N.x * N.y * a;
        R = vec3(1.0 - N.x * N.x * a, b, -N.x);
        U = vec3(b, 1.0 - N.y * N.y * a, -N.y);
    }
    return mat3(R, U, N);
}

void main()
{
    vec3 N = normalize(v_texcoord);
    N.y = -N.y;
    N.z = -N.z;
    //mat3 L2W = BasisFrisvald(N);

    vec3 irradiance = vec3(0);

    vec3 U = vec3(0, 1, 0);
    vec3 R = cross(N, U);
    U = cross(R, N);

    float sampleDelta = 0.01;
    int sampleCount = 0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 V_local = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 V_world = V_local.x * (R) + V_local.y * (U) + V_local.z * (N);
            //vec3 V_world = L2W * V_local;

            irradiance += texture(Cubemap, V_world).xyz * cos(theta) * sin(theta);
            ++sampleCount;
        }
    }
    irradiance = irradiance * PI * (1 / float(sampleCount));

    frag_color = vec4(irradiance, 1);
}
