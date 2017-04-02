#version 400
in vec4 v_color;
in vec2 v_texcoord;

uniform sampler2D DiffuseTexture;

out vec4 frag_color;

void main() {
    vec4 diffuse = texture(DiffuseTexture, v_texcoord);
    frag_color = diffuse * v_color;
}
