#version 400

layout(location=0) in vec3 in_position;
layout(location=1) in vec2 in_texcoord;

out vec2 v_texcoord;

void main()
{
    v_texcoord = in_texcoord;
    gl_Position = vec4(in_position.xy, 1.0, 1.0);
}
