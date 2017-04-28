#version 400

in vec3 v_texcoord;

uniform samplerCube Skybox;

out vec4 frag_color;

void main()
{
    vec3 sky = pow(texture(Skybox, v_texcoord).xyz, vec3(2.2));
    sky = sky / (sky + vec3(1));
    sky = pow(sky, vec3(1/2.2));

    frag_color = vec4(sky, 1);
}
