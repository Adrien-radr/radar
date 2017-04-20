#version 400

in vec2 v_texcoord;
in vec4 v_color;

uniform sampler2D Texture0;
uniform vec4 Color;

out vec4 frag_color;

void main()
{
    vec4 TexValue = texture(Texture0, v_texcoord);
    frag_color = Color;
    frag_color.a *= TexValue.r;
}
