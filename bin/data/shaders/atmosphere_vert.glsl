#version 400

layout(location=0) in vec2 in_position;
layout(location=1) in vec2 in_texcoord;

uniform mat4 InverseModelMatrix;
uniform mat4 InverseProjMatrix;

out vec2 v_texcoord;
out vec3 v_eyeRay;

void main()
{
    v_texcoord = in_texcoord;
    v_eyeRay = ((InverseModelMatrix) * vec4(((InverseProjMatrix) * vec4(in_position.xy, 0.0, 1.0)).xyz,0.0)).xyz;
    gl_Position = vec4(in_position, 1.0, 1.0);
}
