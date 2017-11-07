#version 400

in vec2 v_texcoord;

uniform sampler2D HDRFB;
uniform float Exposure;

out vec4 frag_color;

void main()
{
    vec3 hdrColor = texture(HDRFB, v_texcoord).rgb;

    // Tone mapping and Gamma correction
    //hdrColor = hdrColor / (hdrColor + vec3(1));
    hdrColor = vec3(1.0) - exp(-hdrColor * Exposure);
    hdrColor = pow(hdrColor, vec3(1.0/2.2));

    frag_color = vec4(hdrColor, 1.0);
}
