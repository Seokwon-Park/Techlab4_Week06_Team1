#include "EnginePCH.h"
#include "MovementComponent.h"
#include "GameFramework/Actor.h"

void UMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (UpdatedComponent == nullptr)
	{		
		if (AActor* MyActor = GetOwner())
		{
			if (USceneComponent* NewUpdatedCompoent = MyActor->GetRootComponent())
			{
				SetUpdatedComponent(NewUpdatedCompoent);
			}
		}
	}
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* NewComponent)
{
	if (NewComponent == nullptr)
	{
		return;
	}

	UpdatedComponent = NewComponent;
}

void UMovementComponent::UpdateComponentVelocity()
{
	if (UpdatedComponent)
	{
		UpdatedComponent->ComponentVelocity = Velocity;
	}
}

bool UMovementComponent::MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep)
{
	if (UpdatedComponent)
	{
		UpdatedComponent->MoveComponent(Delta, NewRotation, bSweep);
	}

	return false;
}
