#version 400

layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec4 color;

uniform mat4 ProjMatrix;

out vec2 v_texcoord;
out vec4 v_color;

void main()
{
    v_texcoord = texcoord;
    v_color = color;

    gl_Position = ProjMatrix * vec4(position, 1.0);
}
