#version 400

in vec2 v_texcoord;
in vec4 v_color;

out vec4 frag_color;

void main()
{
    frag_color = v_color + vec4(0.00001*v_texcoord, 0, 1);
}
