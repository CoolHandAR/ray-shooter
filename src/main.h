#pragma once

#include <stdint.h>

uint64_t Engine_GetTicks();
float Engine_GetDeltaTime();
void Engine_SetTimeScale(float scale);