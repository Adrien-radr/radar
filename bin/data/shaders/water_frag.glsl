#version 400
in vec3 v_position;
in vec2 v_texcoord;
in vec3 v_normal;
in vec3 v_halfvector;
in vec3 v_sundirection;

uniform samplerCube Skybox;
uniform vec4 LightColor;
uniform vec3 CameraPos;

out vec4 frag_color;

void main()
{
    vec3 N = normalize(v_normal);
    vec3 L = normalize(v_sundirection);
    vec3 I = normalize(CameraPos - v_position);
    vec3 H = normalize(I + L);

    vec4 emissive_color = vec4(1.0, 1.0, 1.0,  1.0);
	vec4 ambient_color  = vec4(0.0, 0.65, 0.75, 1.0);
	vec4 diffuse_color  = vec4(0.5, 0.65, 0.75, 1.0);
	vec4 specular_color = vec4(0.2, 0.45, 0.53,  1.0);

	float emissive_contribution = 0.00;
	float ambient_contribution  = 0.30;
	float diffuse_contribution  = 0.50;
	float specular_contribution = 1.80;

    float d = max(0.0, dot(L, N));
    bool facing = d > 0.0;

    vec3 reflect_vec = reflect(I, N);
    vec4 reflect_color = texture(Skybox, reflect_vec);
    reflect_color = 0.5 * reflect_color + vec4(0.5, 0.5, 0.5, 0);

    frag_color = emissive_color * emissive_contribution +
                 ambient_color * ambient_contribution * reflect_color +
                 diffuse_color * diffuse_contribution * reflect_color * max(0.0, d) +
                 (facing ? specular_color * specular_contribution * reflect_color * max(0.0, pow(dot(H, N), 120.0)):
                 vec4(0.0, 0.0, 0.0, 0.0));
    frag_color.a = 1.0;
}

