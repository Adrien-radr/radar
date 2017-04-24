#version 400

in vec3 v_texcoord;

uniform sampler2D Envmap;

out vec4 frag_color;

// 1/2pi and 1/pi for phi and theta respectively
const vec2 SphericalNorm = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= SphericalNorm;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(v_texcoord));
    vec3 color = texture(Envmap, uv).xyz;
    frag_color = vec4(color, 1);
}
