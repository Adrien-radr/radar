#version 400

layout(location=0) in vec2 in_position;
layout(location=1) in vec2 in_texcoord;

uniform mat4 WaterProjMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;
out vec2 v_texcoord;
out float v_depth;

float intersectPlane(in vec3 N, in vec3 p0, in vec3 org, in vec3 dir)
{
    float denom = dot(N, dir);
    if(abs(denom) > 1e-5)
    {
        vec3 p0org = p0-org;
        float t = dot(p0org, N) / denom;
        if(t >= 0)
            return t;
        return -1;
    }
    return -1;
}

void main()
{
    v_texcoord = in_texcoord;

    mat4 WPM = ViewMatrix;
    vec4 eyeRay = inverse(WaterProjMatrix) * vec4((inverse(ProjMatrix) * vec4(in_position.xy, 0, 1)).xyz, 0.0);
    vec3 E = eyeRay.xyz;
    vec3 CamPos = vec3(WaterProjMatrix[0][3], WaterProjMatrix[1][3], WaterProjMatrix[2][3]);

    float depth = v_texcoord.y*10;//intersectPlane(vec3(0,1,0), vec3(0,0,0), CamPos, E);
    if(depth < 0.0) depth = 0.0;

    // Matrix projecting the position in the direction of the Eye Ray with depth depending on the vertex
    // height on the screen (vertices at the top are projected farther away)
    vec3 T = CamPos + vec3(E.x*depth,E.y*depth,E.z*depth);
    mat4 MM  = mat4(1,0,0,0,
                    0,1,0,0,
                    0,0,1,0,
                    T,1);

    vec4 v_3d = MM*inverse(WaterProjMatrix)*inverse(ProjMatrix) * vec4(in_position.xy, 0, 1);
    gl_Position = ProjMatrix * WaterProjMatrix * v_3d;
    v_depth = depth;
}
