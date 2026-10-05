#pragma once

#include "Math/Vector4.h"

struct FFogInfo
{
    float    FogDensity = 0.02f;
    float    FogHeightFalloff = 0.2f;
    float    StartDistance = 0.0f;
    float    FogCutoffDistance = 0.0f;
    float    FogMaxOpacity = 1.0f;
    FVector4 FogColor = { 0.5f, 0.6f, 0.7f, 1.0f };
	float fogHeight = 3.0f;
};