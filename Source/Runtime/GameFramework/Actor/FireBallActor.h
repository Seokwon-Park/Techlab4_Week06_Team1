#pragma once
#include "GameFramework/Actor.h"
#include "Component/FireBallComponent.h"

class AFireBallActor : public AActor
{
	DECLARE_CLASS(AFireBallActor, AActor)

public:
	AFireBallActor();
	UFireBallComponent* GetFireBallComponent() const { return FireballComponent; }

private:
	UFireBallComponent* FireballComponent;
};