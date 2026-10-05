#pragma once
#include "Core/Types.h"
#include "Math/Vector.h"

class UPrimitiveComponent;

struct FHitResult
{
	int32 FaceIndex; // Hitted face index

	float Time; // For swept
	float Distance = FLT_MAX;

	FVector Location = FVector(); // For swept
	FVector ImpactPoint = FVector();
	FVector Normal; //For swept
	FVector ImpactNormal;

	FVector TraceStart;
	FVector TraceEnd;

	float PenetrationDepth;

	bool bBlockingHit;
	bool bStartPentrating;

	UPrimitiveComponent* HitComponent = nullptr;	
};