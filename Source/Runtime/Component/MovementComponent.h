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

public:
	// ActorComponent  Interface
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;


	virtual void SetUpdatedComponent(USceneComponent* NewComponent);
	virtual void UpdateComponentVelocity();

	virtual bool MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep);

	virtual void StopMovementImmediately();
};


inline void UMovementComponent::StopMovementImmediately()
{
	Velocity = FVector::ZeroVector;
	UpdateComponentVelocity();
}