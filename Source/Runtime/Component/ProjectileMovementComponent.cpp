#include "EnginePCH.h"
#include "ProjectileMovementComponent.h"

UProjectileMovementComponent::UProjectileMovementComponent()
{
	// Default
	bInitialVelocityInLocalSpace = true;
	bSimulationEnabled = true;
	bSweepCollision = true;

	Velocity = FVector(1.0f, 0.0f, 0.0f);

	ProjectileGravityScale = 1.0f;

	Bounciness = 0.6f;
	Friction = 0.2f;

	HomingAccelerationMagnitude = 0.0f;
}

void UProjectileMovementComponent::InitializeComponent()
{	
	Super::InitializeComponent();

	if (Velocity.SizeSquared() > 0.0f)
	{
		if (InintialSpeed > 0.0f)
		{
			Velocity = Velocity.Normalized() * InintialSpeed;
		}

		if (bInitialVelocityInLocalSpace)
		{
			SetVelocityInLocalSpace(Velocity);
		}

		if (bRotationFollowsVelocity)
		{

		}

		UpdateComponentVelocity();
	}

}

void UProjectileMovementComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	
	if (UpdatedComponent == nullptr)
	{
		return;
	}

	AActor* ActorOwner = UpdatedComponent->GetOwner();
	if (ActorOwner == nullptr)
	{
		return;
	}

}

void UProjectileMovementComponent::SetVelocityInLocalSpace(FVector NewVelocity)
{
	if (UpdatedComponent)
	{
		Velocity = UpdatedComponent->GetWorldMatrix().TransformVectorNoScale(NewVelocity);
	}
}

FVector UProjectileMovementComponent::LimitVelocity(FVector NewVelocity) const
{
	return FVector();
}

FVector UProjectileMovementComponent::ComputeVelocity(FVector InitialVelocity, float DeltaTime) const
{
	// v = v0 + a*t
	const FVector Acceleration = ComputAcceleration(InitialVelocity, DeltaTime);
	FVector NewVelocity = InitialVelocity + (Acceleration * DeltaTime);

	return FVector();
}

FVector UProjectileMovementComponent::ComputeMoveDelta(const FVector& InVelocity, float DeltaTime) const
{
	// p = p0 + v0*t + 1/2*a*t^2

	const FVector NewVelocity = ComputeVelocity(InVelocity, DeltaTime);
	const FVector Delta = (InVelocity * DeltaTime) + (NewVelocity - InVelocity) * (0.5f * DeltaTime);

	return Delta;
}

FVector UProjectileMovementComponent::ComputAcceleration(const FVector& InVelocity, float DeltaTime) const
{
	FVector Acceleration(FVector::ZeroVector);	

	Acceleration.Z += Gravity;
	
	// TODO : Pending

	if (bIsHomingProjectile && HomingTargetComponent != nullptr)
	{
		Acceleration += ComputeHomingAcceleration(InVelocity, DeltaTime);
	}

	return Acceleration;
}

FVector UProjectileMovementComponent::ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const
{
	FVector HomingAcceleration = (HomingTargetComponent->GetWorldLocation() - UpdatedComponent->GetWorldLocation()).Normalized();
	return HomingAcceleration;
}

void UProjectileMovementComponent::AddForce(FVector Force)
{
	PendingForce += Force;
}

FVector UProjectileMovementComponent::GetPendingForce() const
{
	return PendingForce;
}

void UProjectileMovementComponent::ClearPendingForce(bool bClearImmediateForce)
{
	PendingForce = FVector::ZeroVector;
}
