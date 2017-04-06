#version 400

in vec2 v_texcoord;

uniform sampler2D DiffuseTexture; // From GBuffer in Pass1

layout(location=0) out vec4 frag_color;
const float Kernel[9] = float[](
        1 / 16.0, 2 / 16.0, 1 / 16.0,
        2 / 16.0, 4 / 16.0, 2 / 16.0,
        1 / 16.0, 2 / 16.0, 1 / 16.0
        );

void main()
{
    float fragDepth = texture(DiffuseTexture, v_texcoord).r;
    if(fragDepth == 0.0f) discard;

    float off = (1/50.0) / fragDepth;
    vec2 offsets[9] = vec2[](
            vec2(-off, off), vec2(0, off), vec2(off, off),
            vec2(-off, 0), vec2(0, 0), vec2(off, 0),
            vec2(-off, -off), vec2(0, -off), vec2(off, -off)
            );

    float depthArr[9];
    for(int i = 0; i < 9; ++i)
    {
        depthArr[i] = texture(DiffuseTexture, v_texcoord + offsets[i]).r;
    }


    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; ++i)
    {
        color += depthArr[i] * Kernel[i];
    }

    frag_color = vec4(color, 1.0);
}
