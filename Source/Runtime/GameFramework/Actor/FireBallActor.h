#pragma once
#include "GameFramework/Actor.h"
#include "Component/FireBallComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/RotatingMovementComponent.h"
#include "Component/ProjectileMovementComponent.h"

class AFireBallActor : public AActor
{
	DECLARE_CLASS(AFireBallActor, AActor)

public:
	AFireBallActor();
	UFireBallComponent* GetFireBallComponent() const { return FireballComponent; }

private:
	UFireBallComponent* FireballComponent;
	UStaticMeshComponent* StaticMeshComponent;

	URotatingMovementComponent* RotatingMovementComponent;
	UProjectileMovementComponent* ProjectileMovementComponent;
};