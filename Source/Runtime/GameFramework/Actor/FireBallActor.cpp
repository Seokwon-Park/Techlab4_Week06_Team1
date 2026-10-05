#include "EnginePCH.h"
#include "FireBallActor.h"

AFireBallActor::AFireBallActor()
{
	FireballComponent = CreateDefaultSubobject<UFireBallComponent>("UFireBallComponent");
	FireballComponent->SetupAttachment(GetRootComponent());
}