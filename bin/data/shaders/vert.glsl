#version 400

layout(location=0) in vec3 in_position;
layout(location=1) in vec2 in_texcoord;
layout(location=2) in vec3 in_normal;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

out vec3 v_position;
out vec2 v_texcoord;
out vec3 v_normal;

void main()
{
    v_texcoord = in_texcoord;
    v_normal = mat3(transpose(inverse(ModelMatrix))) * in_normal;
    vec4 world_position = ModelMatrix * vec4(in_position, 1.0);
    v_position = world_position.xyz;
    gl_Position = ProjMatrix * ViewMatrix * world_position;
}

/*
in vec3 in_offset;

void main() {
    vec4 p = ProjMatrix * vec4(in_offset, 1.0) + vec4(in_position, 0.0);
    gl_Position = p; 
    // gl_Position = vec4(p.x, p.y, 0, p.w);
}
*/
