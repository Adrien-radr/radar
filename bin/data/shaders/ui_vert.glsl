#version 400

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;

uniform mat4 ProjMatrix;

out vec2 v_texcoord;

void main()
{
    v_texcoord = texcoord;

    gl_Position = ProjMatrix * vec4(position, 1.0);
}
