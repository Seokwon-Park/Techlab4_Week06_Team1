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

void UMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}
  
void UMovementComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
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

FVector UMovementComponent::ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& HIt) const
{
	return FVector();
}

bool UMovementComponent::MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* Hit)
{
	if (UpdatedComponent)
	{
		return UpdatedComponent->MoveComponent(Delta, NewRotation, bSweep, Hit);		
	}

	return false;
}
