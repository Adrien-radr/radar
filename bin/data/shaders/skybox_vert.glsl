#version 400

layout(location=0) in vec3 in_position;

uniform mat4 ViewMatrix;
uniform mat4 ProjMatrix;

out vec3 v_texcoord;

void main()
{
    v_texcoord = in_position; // unit cube positions are cartesian unit direction
    vec4 pos = ProjMatrix * ViewMatrix * vec4(in_position, 1.0);
    gl_Position = pos.xyww; // always max depth
}
