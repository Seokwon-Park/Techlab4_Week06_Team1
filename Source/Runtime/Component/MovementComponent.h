#pragma once
#include "ActorComponent.h"
#include "SceneComponent.h"

class UMovementComponent : public UActorComponent
{
	DECLARE_CLASS(UMovementComponent, UActorComponent)
	REFLECT_START(ClassName)
		REFLECT_END()

protected:
	USceneComponent* UpdatedComponent;

	FVector Velocity;

	// TEMP Physics
	float Gravity = 9.81f;

public:
	// ActorComponent  Interface
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;

	virtual void SetUpdatedComponent(USceneComponent* NewComponent);
	virtual void UpdateComponentVelocity();

	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& HIt) const;

	virtual bool MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* Hit);

	virtual void StopMovementImmediately();
};


inline void UMovementComponent::StopMovementImmediately()
{
	Velocity = FVector::ZeroVector;
	UpdateComponentVelocity();
}