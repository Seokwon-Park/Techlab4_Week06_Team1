#include  "EnginePCH.h"
#include "ExponentialHeightFogComponent.h"

FFogInfo UExponentialHeightFogComponent::GetFogInfo() const
{
	FFogInfo FogInfo;
	FogInfo.FogDensity = FogDensity;
	FogInfo.FogHeightFalloff = FogHeightFalloff;
	FogInfo.StartDistance = StartDistance;
	FogInfo.FogCutoffDistance = FogCutoffDistance;
	FogInfo.FogMaxOpacity = FogMaxOpacity;
	FogInfo.FogColor = FogColor;

	return FogInfo;
}
