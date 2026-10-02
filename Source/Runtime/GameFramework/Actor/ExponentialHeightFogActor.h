#pragma once

#include "GameFramework/Actor.h"
#include "Component/BillboardComponent.h"
#include "Component/ExponentialHeightFogComponent.h"


class AExponentialHeightFogActor final : public AActor
{
	DECLARE_CLASS(AExponentialHeightFogActor, AActor)


public:
	AExponentialHeightFogActor();
	virtual ~AExponentialHeightFogActor() override = default;

	AExponentialHeightFogActor(const AExponentialHeightFogActor&) = delete;
	AExponentialHeightFogActor operator=(const AExponentialHeightFogActor&) = delete;

private:
	UExponentialHeightFogComponent* ExponentialHeightFogComponent = nullptr;
	UBillboardComponent* BillboardComponent = nullptr;
};