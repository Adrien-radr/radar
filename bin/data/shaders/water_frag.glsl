#version 400

in vec2 v_texcoord;
in float v_depth;

out vec4 frag_color;

void main()
{
    float alpha = 1.0;
    if(v_depth < 0.0) discard;
    float depthmax = 10;
    float depthclamp = 100;
    if(v_depth > depthmax) alpha = 1.0 - ((min(depthclamp,v_depth) - depthmax) / (depthclamp - depthmax));
    float color = pow(v_depth,1/2.2);
    frag_color = vec4(color, color, color, alpha);
}
