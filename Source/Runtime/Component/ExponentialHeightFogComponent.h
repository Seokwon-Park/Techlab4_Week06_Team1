#pragma once
#include "SceneComponent.h"


class UExponentialHeightFogComponent final : public USceneComponent
{
	DECLARE_CLASS(UExponentialHeightFogComponent, USceneComponent)
	REFLECT_START(UExponentialHeightFogComponent)
	REFLECT_END()

public:
	UExponentialHeightFogComponent() = default;
	~UExponentialHeightFogComponent() override = default;

	UExponentialHeightFogComponent(const UExponentialHeightFogComponent&) = delete;
	UExponentialHeightFogComponent& operator=(const UExponentialHeightFogComponent&) = delete;
private:
	//안개의 농도
	float FogDensity;
	//높이 올라갈때 안개가 옅어지는 속도
	float FogHeightFalloff;
	//안개가 시작되는 거리(카메라에서 얼마나 떨어져야 안개가 보이기 시작하는지)
	float StartDistance;
	//안개가 끝나는 거리
	float FogCutoffDistance;
	//안개 불투명도의 상한
	float FogMaxOpacity;

	//안개 내부산란색상
	//FLinearColor FogInscatteringColor;


};