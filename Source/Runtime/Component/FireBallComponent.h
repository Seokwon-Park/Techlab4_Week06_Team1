#pragma once
#include "PrimitiveComponent.h"

class UFireBallComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UFireBallComponent, UPrimitiveComponent)

	REFLECT_START(ClassName)
		PROPERTY(Intensity)
		PROPERTY(Radius)
		PROPERTY(RadiusFalloff)
		PROPERTY_TYPE(LightColor, Color)
		REFLECT_END()

public:
	UFireBallComponent() = default;
	virtual ~UFireBallComponent() = default;

	float GetIntensity() const { return Intensity; }
	float GetRadius() const { return Radius; }
	float GetRadiusFalloff() const { return RadiusFalloff; }
	const FVector4& GetLightColor() const { return LightColor; }


private:
	float Intensity = 1.0f;
	float Radius = 6.5f;
	float RadiusFalloff = 1.6f;
	FVector4 LightColor = FVector4(1.0f, 0.0f, 0.0f, 1.0f);
};
