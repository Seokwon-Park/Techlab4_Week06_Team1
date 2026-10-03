#pragma once
#include "SceneComponent.h"
#include "Render/Foginfo.h"


class UExponentialHeightFogComponent final : public USceneComponent
{
	DECLARE_CLASS(UExponentialHeightFogComponent, USceneComponent)
	REFLECT_START(UExponentialHeightFogComponent)
		PROPERTY(FogDensity)
		PROPERTY(FogHeightFalloff)
		PROPERTY(StartDistance)
		PROPERTY(FogCutoffDistance)
		PROPERTY(FogMaxOpacity)
		PROPERTY(FogColor)
	REFLECT_END()

public:
	UExponentialHeightFogComponent() = default;
	~UExponentialHeightFogComponent() override = default;

	UExponentialHeightFogComponent(const UExponentialHeightFogComponent&) = delete;
	UExponentialHeightFogComponent& operator=(const UExponentialHeightFogComponent&) = delete;

	FFogInfo GetFogInfo() const;

private:
	//포그의 밀도, 높이에 따른 감소율, 시작 거리, 최대 불투명도, 색상 등을 설정하는 변수들
	float    FogDensity = 5.0f;
	float    FogHeightFalloff = 0.2f;
	float    StartDistance = 0.0f;
	float    FogCutoffDistance = 0.0f;
	float    FogMaxOpacity = 1.0f;
	FVector4 FogColor = { 1.0f, 0.4f, 0.7f, 1.0f };


};