#version 400
in vec2 v_texcoord;

uniform sampler2D DiffuseTexture;
uniform vec4 TextColor;

out vec4 frag_color;

void main() {
    vec4 diffuse = texture(DiffuseTexture, v_texcoord);
    frag_color = vec4(TextColor.xyz, diffuse.x);
}
