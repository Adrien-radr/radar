#version 400

// NOTE - in_position.w is the wanted PointSize at z=1
layout(location=0) in vec4 in_position;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
//uniform mat4 ModelMatrix;

out vec3 v_eyeSpace_pos;
out vec3 v_eyeSpace_LightDir;

void main()
{
    vec3 LightDir = normalize(vec3(0.5, 0.2, 1.0));
    v_eyeSpace_LightDir = transpose(inverse(mat3(ViewMatrix))) * LightDir;

    vec4 eyeSpace_pos = ViewMatrix * vec4(in_position.xyz, 1.0);
    v_eyeSpace_pos = eyeSpace_pos.xyz;
    gl_Position = ProjMatrix * eyeSpace_pos;
    gl_PointSize = in_position.w / gl_Position.z;
}

