#version 400

in vec3 v_texcoord;

uniform samplerCube Skybox;

out vec4 frag_color;

void main()
{
    frag_color = vec4(1.0, 1.0, 1.0, 1.0) * texture(Skybox, -v_texcoord);
}
