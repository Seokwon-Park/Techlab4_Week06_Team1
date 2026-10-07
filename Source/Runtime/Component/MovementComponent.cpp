#include "EnginePCH.h"
#include "MovementComponent.h"
#include "GameFramework/Actor.h"

void UMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (AActor* MyActor = GetOwner())
	{
		if (USceneComponent* NewUpdatedCompoent = MyActor->GetRootComponent())
		{
			SetUpdatedComponent(NewUpdatedCompoent);
		}
	}

	PrimaryComponentTick.bCanEverTick = true;
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

void UMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{

}

FVector UMovementComponent::ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& HIt) const
{
	if (!bConstrainToPlane)
	{
		return FVector::VectorPlaneProject(Delta, Normal) * Time;		
	}
	else
	{
		const FVector ProjectedNormal = ConstrainDirectionToPlane(Normal);
		return FVector::VectorPlaneProject(Delta, ProjectedNormal) * Time;
	}
}

bool UMovementComponent::MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit)
{
	if (UpdatedComponent)
	{
		const FVector NewDelta = ConstrainDirectionToPlane(Delta);
		return UpdatedComponent->MoveComponent(NewDelta, NewRotation, bSweep, OutHit);
	}

	return false;
}

bool UMovementComponent::SafeMoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult& OutHit)
{
	if(UpdatedComponent == nullptr)
	{
		OutHit.Init();
		return false;
	}

	return false;
}

void UMovementComponent::SetPlaneConstraintNormal(FVector PlaneNormal)
{
	PlaneConstraintNormal = PlaneNormal.GetSafeNormal();
	// Contraint Axis Setting (in unreal)
}

void UMovementComponent::SetPlaneConstraintFromVectors(FVector Forward, FVector Up)
{
	PlaneConstraintNormal = (Up ^ Forward).GetSafeNormal();
}

void UMovementComponent::SetPlaneConstraintOrigin(FVector PlaneOrigin)
{
	PlaneConstraintOrigin = PlaneOrigin;
}

void UMovementComponent::SetPlaneConstraintEnabled(bool bEnabled)
{
	bConstrainToPlane = bEnabled;
}

const FVector& UMovementComponent::GetPlaneConstraintNormal() const
{
	return PlaneConstraintNormal;
}

const FVector& UMovementComponent::GetPlaneConstraintOrigin() const
{
	return PlaneConstraintOrigin;
}

void UMovementComponent::StopMovementImmediately()
{
	Velocity = FVector::ZeroVector;
	UpdateComponentVelocity();
}

FVector UMovementComponent::ConstrainDirectionToPlane(FVector Direction) const
{
	if (bConstrainToPlane)
	{
		Direction = FVector::VectorPlaneProject(Direction, PlaneConstraintNormal);
	}

	return Direction;
}

FVector UMovementComponent::ConstrainLocationToPlane(FVector Location) const
{
	if (bConstrainToPlane)
	{
		Location = FVector::PointPlaneProject(Location, PlaneConstraintOrigin, PlaneConstraintNormal);
	}

	return Location;
}

FVector UMovementComponent::ConstrainNormalToPlane(FVector Normal) const
{
	if (bConstrainToPlane)
	{
		Normal = FVector::VectorPlaneProject(Normal, PlaneConstraintNormal).GetSafeNormal();		
	}

	return Normal;
}
