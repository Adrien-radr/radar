#ifndef TESTS_H
#define TESTS_H

#include "definitions.h"
#include "Game/sun.h"

#define TEST_MODELS 0
#define TEST_SPHEREARRAY 1
#define TEST_PBRMATERIALS 1
#define TEST_SKYBOX 0
#define TEST_PLANE 0
#define TEST_SOUND 0

namespace Tests {
void Destroy();
void ReloadShaders(rf::context *Context);
bool Init(rf::context *Context, config const *Config);
void Render(game::state *State, rf::input *Input, rf::context *Context);
}
#endif
