#pragma once
#include "SceneComponent.h"

class UFireBallComponent : public USceneComponent
{
	DECLARE_CLASS(UFireBallComponent, USceneComponent)

	REFLECT_START(ClassName)
		PROPERTY(Intensity)
		PROPERTY(Radius)
		PROPERTY(RadiusFalloff)
		PROPERTY_TYPE(LightColor, Color)
		REFLECT_END()

public:
	UFireBallComponent() = default;
	virtual ~UFireBallComponent() override;
	void InitializeComponent();

	float GetIntensity() const { return Intensity; }
	float GetRadius() const { return Radius; }
	float GetRadiusFalloff() const { return RadiusFalloff; }
	const FVector4& GetLightColor() const { return LightColor; }


private:
	float Intensity = 1.8f;
	float Radius = 6.5f;
	float RadiusFalloff = 3.0f;
	FVector4 LightColor = FVector4(1.0f, 0.0f, 0.8f, 1.0f);
};
