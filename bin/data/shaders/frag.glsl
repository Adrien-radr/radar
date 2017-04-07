#version 400
in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;

uniform sampler2D DiffuseTexture;
uniform samplerCube Skybox;
uniform vec4 LightColor;
uniform vec3 CameraPos;

out vec4 frag_color;

void main()
{
    vec4 diffuse_texture = texture(DiffuseTexture, v_texcoord);

    vec3 N = normalize(v_normal);

    vec3 LightDir = normalize(vec3(0.5, 0.2, 1.0));

    vec4 Ka = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 Kd = vec4(1.0, 1.0, 1.0, 1.0);

    vec4 La = vec4(0.2, 0.2, 0.2, 1.0);
    vec4 Ld = LightColor;

    vec4 ambient = Ka * La;
    vec4 diffuse = Kd * (Ld * max(0.0, dot(LightDir, N)));

    vec3 I = normalize(CameraPos - v_position);
    vec3 reflect_vec = reflect(I, N);
    vec4 reflect_color = texture(Skybox, reflect_vec);

    float IOR = 1.00 / 1.33; // air to water
    vec3 refract_vec = refract(I, N, IOR);
    vec4 refract_color = texture(Skybox, refract_vec);

    frag_color = (ambient + diffuse) * (0.025 * reflect_color + 0.95 * refract_color + 0.05 * diffuse_texture);
}
