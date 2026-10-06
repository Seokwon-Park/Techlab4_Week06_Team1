#include "EnginePCH.h"

#include "GameFramework/Actor.h"

UActorComponent::~UActorComponent()
{
    if (Owner)
    {
        Owner->RemoveOwnedComponent(this);
    }
}

void UActorComponent::SetActive(bool bNewActive)
{
    bIsActive = bNewActive;
}
