#pragma once
#include "ActorComponent.h"
#include "SceneComponent.h"

class UPrimitiveComponent;

class UMovementComponent : public UActorComponent
{
	DECLARE_CLASS(UMovementComponent, UActorComponent)
	REFLECT_START(ClassName)
		REFLECT_END()

protected:
	USceneComponent* UpdatedComponent;
	UPrimitiveComponent* UpdatedPrimitive = nullptr;

public:
	FVector Velocity;
	
	float Gravity = 9.81f;

	// ActorComponent  Interface
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;

	virtual void SetUpdatedComponent(USceneComponent* NewComponent);
	virtual void UpdateComponentVelocity();
	virtual void StopMovementImmediately();

	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice = 0.0f, const FVector& MoveDelta = FVector::ZeroVector);
	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& HIt) const;

	virtual bool MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit);	

	virtual bool SafeMoveUpdatedComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult& OutHit);

	// Plane Constraint
	virtual void SetPlaneConstraintNormal(FVector PlaneNormal);
	virtual void SetPlaneConstraintFromVectors(FVector Forward, FVector Up);
	virtual void SetPlaneConstraintOrigin(FVector PlaneOrigin);
	virtual void SetPlaneConstraintEnabled(bool bEnabled);
	
	const FVector& GetPlaneConstraintNormal() const;
	const FVector& GetPlaneConstraintOrigin() const;
	
	virtual FVector ConstrainDirectionToPlane(FVector Direction) const;
	virtual FVector ConstrainLocationToPlane(FVector Location) const;
	virtual FVector ConstrainNormalToPlane(FVector Normal) const;

protected:
	bool bConstrainToPlane = false;
	FVector PlaneConstraintNormal = FVector::ZeroVector;
	FVector PlaneConstraintOrigin = FVector::ZeroVector;
	
};
