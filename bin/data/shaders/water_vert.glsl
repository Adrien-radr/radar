#version 400

layout(location=0) in vec2 in_position;
layout(location=1) in vec2 in_texcoord;

uniform mat4 WaterProjMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;
uniform vec3 ProjectorPosition;

uniform float Time;

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
    #if 0


    // Matrix projecting the position in the direction of the Eye Ray with depth depending on the vertex
    // height on the screen (vertices at the top are projected farther away)

    #endif
    vec3 CamPos = ProjectorPosition;//vec3(WPM[3][0], WPM[3][1], WPM[3][2]);
    vec4 eyeRay = inverse(WPM) * vec4((inverse(ProjMatrix) * vec4(in_position.xy, 0, 1)).xyz, 0.0);
    vec3 E = normalize(eyeRay.xyz);

    float depth = intersectPlane(vec3(0,1,0), vec3(0,3,0), CamPos, E);
    if(depth < 0.0) 
    {
        depth = 0.0;
        v_depth = -1000;
    }
    else
    {
        v_depth = depth;
    }

    E *= depth;

    //vec4 v_3d = inverse(MM*WPM) * inverse(ProjMatrix) * vec4(in_position.xy, 0, 1);
    float sf = 5.0;
    mat4 ScaleMat = mat4(
        sf,0,0,0,
        0,1.0,0,0,
        0,0,sf,0,
        0,0,0,1.0
    );

    vec4  v_3d = ScaleMat * vec4(CamPos + E, 1);
    //v_3d.y += 0.1*sin(Time);
    gl_Position = ProjMatrix * WPM * v_3d;
}
