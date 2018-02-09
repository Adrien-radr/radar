#version 400

in vec2 v_texcoord;
in float v_depth;

out vec4 frag_color;

void main()
{
    float color = pow(v_depth/1, 1/1.2);
    frag_color = vec4(color, color, color, 1.0);
}
