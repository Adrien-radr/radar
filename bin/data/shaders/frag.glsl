#version 400
in vec2 v_texcoord;

uniform sampler2D DiffuseTexture;

out vec4 frag_color;

void main() {
    vec4 diffuse = texture(DiffuseTexture, v_texcoord);
    frag_color = diffuse;
}
