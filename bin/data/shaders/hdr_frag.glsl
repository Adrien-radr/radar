#version 400

#define ACCURATE_GAMMA

#ifndef FXAA_REDUCE_MIN
    #define FXAA_REDUCE_MIN   (1.0/ 4.0)
#endif
#ifndef FXAA_REDUCE_MUL
    #define FXAA_REDUCE_MUL   (1.0 / 2.0)
#endif
#ifndef FXAA_SPAN_MAX
    #define FXAA_SPAN_MAX     16.0
#endif

in vec2 v_texcoord;

uniform sampler2D HDRFB;
uniform float MipmapQueryLevel;
uniform vec2  Resolution;

out vec4 frag_color;

// Parameters for Camera control
// Relative aperture (N, in f-stops) -> controls how wide the aperture is opened and impacts the DoF.
// Shutter time (t, in seconds) -> controls the duration of aperture opening and impacts motion blur.
// Sensor sensitivity (S, in ISO) -> controls how photons are quantized on the digital sensor.

// Exposure Value(EV) for ISO 100 (EV100) is log_2(N^2 / t) - log_2(S / 100).
float ComputeEV100FromAvgLuminance(float avgLuminance)
{
    return log2(avgLuminance * 100.0f / 12.5f);
}

float ConvertEV100ToExposure(float EV100)
{
    float maxLuminance = 1.2f * pow(2.0f, EV100);
    return 1.0f / maxLuminance;
}

void GetTexcoords(vec2 fragCoord, out vec2 v_rgbNE, out vec2 v_rgbNW, out vec2 v_rgbSE, out vec2 v_rgbSW, out vec2 v_rgbM)
{
    vec2 inverseVP = 1.0 / Resolution;
    v_rgbNE = (fragCoord + vec2(1, 1)) * inverseVP;
    v_rgbNW = (fragCoord + vec2(-1, 1)) * inverseVP;
    v_rgbSE = (fragCoord + vec2(1, -1)) * inverseVP;
    v_rgbSW = (fragCoord + vec2(-1, -1)) * inverseVP;
    v_rgbM = (fragCoord) * inverseVP;
}

vec3 ToneMapping(vec3 Col, float Exposure)
{
    return vec3(1.0) - exp(-Col * Exposure);
}

vec3 GammaCorrection(vec3 Col)
{
#ifdef ACCURATE_GAMMA
    float Y = dot(Col, vec3(0.2126, 0.7152, 0.0722));
    vec3 srgblo = Col * 12.92;
    vec3 srgbhi = (pow(abs(Col), vec3(1.0/2.4)) * 1.055) - 0.055;
    vec3 srgb = (Y <= 0.0031308) ? srgblo : srgbhi;
    return srgb;
#else
    return pow(Col, vec3(1.0/2.2));
#endif
}

vec3 PostProcess(vec3 Col, float Exposure)
{
    Col = ToneMapping(Col, Exposure);
    Col = GammaCorrection(Col);
    return Col;
}

vec3 fxaa(sampler2D tex, float Exposure, vec2 fragCoord, vec2 resolution,
            vec2 v_rgbNW, vec2 v_rgbNE, 
            vec2 v_rgbSW, vec2 v_rgbSE, 
            vec2 v_rgbM) {
    vec3 color;
    mediump vec2 inverseVP = vec2(1.0 / resolution.x, 1.0 / resolution.y);
    vec3 rgbNW = PostProcess(texture2D(tex, v_rgbNW).xyz, Exposure);
    vec3 rgbNE = PostProcess(texture2D(tex, v_rgbNE).xyz, Exposure);
    vec3 rgbSW = PostProcess(texture2D(tex, v_rgbSW).xyz, Exposure);
    vec3 rgbSE = PostProcess(texture2D(tex, v_rgbSE).xyz, Exposure);
    vec3 rgbM = PostProcess(texture2D(tex, v_rgbM).xyz, Exposure);
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    mediump vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) * inverseVP;
    
    vec3 rgbA = 0.5 * (
        texture2D(tex, fragCoord * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture2D(tex, fragCoord * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture2D(tex, fragCoord * inverseVP + dir * -0.5).xyz +
        texture2D(tex, fragCoord * inverseVP + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = rgbA;
    else
        color = rgbB;
    return color;
}

void main()
{
    // NOTE - this uses the avg luminance from the current frame for automatic exposure.
    // In practice it is better to keep an histogram of it through frames to avoid flickering.
    vec3 Mean = textureLod(HDRFB, v_texcoord, MipmapQueryLevel).rgb;
    float Y = log(1.0+dot(Mean, vec3(0.2126, 0.7152, 0.0722)));

    float EV100 = ComputeEV100FromAvgLuminance(Y);
    // NOTE - arbitrary value for the min/max Exposure here. Probably this should be derived 
    // according to time of day / weather, etc
    float Exposure = min(10.0,max(0.001,ConvertEV100ToExposure(EV100)));

    // FXAA
    vec2 v_rgbNE, v_rgbNW, v_rgbSW, v_rgbSE, v_rgbM;
    GetTexcoords(gl_FragCoord.xy, v_rgbNE, v_rgbNW, v_rgbSE, v_rgbSW, v_rgbM);

    vec3 hdrColor = fxaa(HDRFB, Exposure, gl_FragCoord.xy, Resolution, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);

    // Remove the auto exposure since its fucking things up, need a better more progressive way of doing i
    float Exp = 1.0;
    //float Exp = Exposure;

    // Tone mapping and Gamma correction
    hdrColor = PostProcess(hdrColor, Exp);

    frag_color = vec4(hdrColor, 1.0);
}
