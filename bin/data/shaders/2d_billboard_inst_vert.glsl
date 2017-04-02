#version 400

layout(location=0) in vec3 in_position;
layout(location=1) in vec2 in_texcoord;
layout(location=2) in vec4 in_color;

uniform mat4 ProjMatrix;
uniform mat4 ModelMatrix;

out vec4 v_color;
out vec2 v_texcoord;

void main()
{
    v_texcoord = in_texcoord;
    v_color = in_color;
    gl_Position = ProjMatrix * ModelMatrix * vec4(in_position, 1.0);
}

/*
in vec3 in_offset;

void main() {
    vec4 p = ProjMatrix * vec4(in_offset, 1.0) + vec4(in_position, 0.0);
    gl_Position = p; 
    // gl_Position = vec4(p.x, p.y, 0, p.w);
}
*/
