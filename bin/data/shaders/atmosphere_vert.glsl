#version 400

layout(location=0) in vec2 in_position;
layout(location=1) in vec2 in_texcoord;

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;
uniform vec3 SunDirection;
uniform float CameraScale;

out vec2 v_texcoord;
out vec3 v_eyeRay;
out vec3 v_sunUV;
out vec3 v_CameraPosition;

void main()
{
    mat4 InvViewMatrix = inverse(ViewMatrix);
    vec4 sunUV = ProjMatrix * ViewMatrix * vec4(SunDirection, 0.0);

    v_texcoord = in_texcoord;
    v_CameraPosition = vec3(InvViewMatrix[3][0], InvViewMatrix[3][1], InvViewMatrix[3][2]) * CameraScale;
    v_eyeRay = (InvViewMatrix * vec4((inverse(ProjMatrix) * vec4(in_position.xy, 0.0, 1.0)).xyz,0.0)).xyz;
    v_sunUV = sunUV.xyz / sunUV.w;
    gl_Position = vec4(in_position, 1.0, 1.0);
}
