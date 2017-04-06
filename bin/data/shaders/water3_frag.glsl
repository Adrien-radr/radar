#version 400

in vec2 v_texcoord;

uniform mat4 ProjMatrix;
uniform sampler2D DiffuseTexture; // From GBuffer in Pass2

out vec4 frag_color;

vec3 ComputeEyePos(vec3 clipPos)
{
    float x = clipPos.x * 2 - 1;
    float y = (clipPos.y) * 2 - 1;
    vec4 projected_pos = vec4(x, y, clipPos.z, 1.0f);
    vec4 pos_vs = inverse(ProjMatrix) * projected_pos;

    return pos_vs.xyz / pos_vs.w;
}

void main()
{
    float fragDepth = texture(DiffuseTexture, v_texcoord).r;
    if(fragDepth == 0.0f) discard;
    frag_color = vec4(fragDepth, fragDepth, fragDepth, 1.0);

#if 0
    float fragDepth = texture(DiffuseTexture, v_texcoord).r;
    if(fragDepth == 0.0f) discard;

    float off = (1/200.0) / fragDepth;
    vec2 offsets[9] = vec2[](
            vec2(-off, off), vec2(0, off), vec2(off, off),
            vec2(-off, 0), vec2(0, 0), vec2(off, 0),
            vec2(-off, -off), vec2(0, -off), vec2(off, -off)
            );

    float depthArr[9];
    vec3 posArr[9];
    for(int i = 0; i < 9; ++i)
    {
        depthArr[i] = texture(DiffuseTexture, v_texcoord + offsets[i]).r;
        posArr[i] = ComputeEyePos(vec3(v_texcoord + offsets[i], depthArr[i]));
    }


    vec3 ddx = posArr[5] - posArr[4];
    vec3 ddx2 = posArr[4] - posArr[3];
    vec3 ddy = posArr[2] - posArr[4];
    vec3 ddy2 = posArr[4] - posArr[7];

    if(abs(ddx.z) > abs(ddx2.z))
        ddx = ddx2;

    if(abs(ddy.z) > abs(ddy2.z))
        ddy = ddy2;

    vec3 n = cross(ddx, ddy);
    n = normalize(n);

    float Kernel[9] = float[](
        1 / 16.0, 2 / 16.0, 1 / 16.0,
        2 / 16.0, 4 / 16.0, 2 / 16.0,
        1 / 16.0, 2 / 16.0, 1 / 16.0
    );

    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; ++i)
    {
        color += depthArr[i] * Kernel[i];
    }

    frag_color = vec4(color, 1.0);
    //frag_color = vec4(n, 1.0);
    //frag_color = vec4(linear_depth, linear_depth, linear_depth, 1.0);
#endif

}
