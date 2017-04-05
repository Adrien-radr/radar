#version 400
in vec2 v_texcoord;
in vec3 v_normal;

uniform sampler2D DiffuseTexture;
uniform vec4 LightColor;

out vec4 frag_color;

void main() {
    vec4 diffuse_texture = texture(DiffuseTexture, v_texcoord);

    vec3 LightDir = normalize(vec3(0.5, 0.2, 1.0));

    vec4 Ka = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 Kd = vec4(1.0, 1.0, 1.0, 1.0);

    vec4 La = vec4(0.2, 0.2, 0.2, 1.0);
    vec4 Ld = LightColor;

    vec4 ambient = Ka * La;
    vec4 diffuse = Kd * (Ld * max(0.0, dot(LightDir, v_normal)));

    frag_color = (ambient + diffuse) * diffuse_texture;
}
