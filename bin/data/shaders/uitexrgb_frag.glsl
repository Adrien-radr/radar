#version 400

in vec2 v_texcoord;
in vec4 v_color;

uniform sampler2D Texture0;

out vec4 frag_color;

void main()
{
    frag_color = texture(Texture0, v_texcoord);
}
