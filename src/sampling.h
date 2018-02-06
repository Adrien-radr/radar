#ifndef SAMPLING_H
#define SAMPLING_H

#include "rf/rf_common.h"

real32 Halton2(uint32 Index);
real32 Halton3(const uint32 Index);
real32 Halton5(const uint32 Index);

real32 VanDerCorput(uint32 bits);

vec2f SampleHammersley(uint32 i, real32 InverseN);
vec3f SampleHemisphereUniform(real32 u, real32 v);
vec3f SampleHemisphereCosine(real32 u, real32 v);

#endif
