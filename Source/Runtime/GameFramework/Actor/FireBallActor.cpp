#include "EnginePCH.h"
#include "FireBallActor.h"

AFireBallActor::AFireBallActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("UStaticMeshComponent");
	StaticMeshComponent->SetupAttachment(GetRootComponent());

	FireballComponent = CreateDefaultSubobject<UFireBallComponent>("UFireBallComponent");
	FireballComponent->SetupAttachment(GetRootComponent());
}