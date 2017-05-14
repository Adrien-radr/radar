#version 400

layout(location=0) in vec3 in_position;
layout(location=1) in vec3 in_normal;
layout(location=2) in vec3 in_texcoord;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;
uniform vec3 SunDirection;

out vec3 v_position;
out vec2 v_texcoord;
out vec3 v_normal;
out vec3 v_sundirection;

void main()
{
    vec4 world_position = ModelMatrix * vec4(in_position, 1.0);

    v_position = world_position.xyz;
    v_texcoord = in_texcoord.xy;
    v_normal = inverse(transpose(mat3(ModelMatrix))) * in_normal;
    v_sundirection = SunDirection;

    gl_Position = ProjMatrix * ViewMatrix * world_position;
}
