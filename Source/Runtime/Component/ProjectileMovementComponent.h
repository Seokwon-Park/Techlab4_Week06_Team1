#pragma once
#include "MovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
	DECLARE_CLASS(UProjectileMovementComponent, UMovementComponent)

		REFLECT_START(ClassName)
		REFLECT_END()

public:
	UProjectileMovementComponent();

	// Projectile
	float InintialSpeed = 0.0f;
	float MaxSpeed = 0.0f;
	float ProjectileGravityScale = 1.0f;

	bool bRotationFollowsVelocity;
	bool bInitialVelocityInLocalSpace;

	// Bounce
	bool bShouldBounce;
	float Bounciness;
	float Friction;	

	// Simulation
	bool bSimulationEnabled;
	bool bSweepCollision;

	// Homing
	bool bIsHomingProjectile;
	float HomingAccelerationMagnitude;
	USceneComponent* HomingTargetComponent = nullptr;

	// Interpolation

public:
	// Interface
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime) override;

	// Projectile
	virtual void SetVelocityInLocalSpace(FVector NewVelocity);

	FVector LimitVelocity(FVector NewVelocity) const;
	virtual FVector ComputeVelocity(FVector InitialVelocity, float DeltaTime) const;
	virtual FVector ComputeMoveDelta(const FVector& InVelocity, float DeltaTime) const;
	virtual FVector ComputAcceleration(const FVector& InVelocity, float DeltaTime) const;
	virtual FVector ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const;

	void AddForce(FVector Force);
	FVector GetPendingForce() const;
	void ClearPendingForce(bool bClearImmediateForce = false);

private:
	FVector PendingForce;
};