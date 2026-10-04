#include "EnginePCH.h"
#include "Component/SceneComponent.h"

#include "GameFramework/Actor.h"

USceneComponent::~USceneComponent()
{
	TArray<USceneComponent*> Children = AttachChildren;
	AttachChildren.Reset();
	for (USceneComponent* Child : Children)
	{
		Child->AttachParent = nullptr;          
		Child->SetupAttachment(AttachParent);   
	}

	if (AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->GetRootComponent() == this)
			OwnerActor->SetRootComponent(Children.Num() > 0 ? Children[0] : nullptr);
	}

	DetachFromParent();
}

void USceneComponent::SetRelativeLocationAndRotation(const FVector& NewLocation, const FRotator& NewRotation)
{
	Transform.Location = NewLocation;
	Transform.Rotation = NewRotation;
	MarkTransformDirty();
}

void USceneComponent::SetupAttachment(USceneComponent* InParent)
{
	if (InParent == this || AttachParent == InParent) return;

	// 순환 체크
	for (USceneComponent* Parent = InParent; Parent != nullptr; Parent = Parent->AttachParent)
		if (Parent == this) return;

	DetachFromParent();
	AttachParent = InParent;
	if (AttachParent)
	{
		AttachParent->AttachChildren.Add(this);
	}
}

void USceneComponent::DetachFromParent()
{
	if (!AttachParent) return;

	TArray<USceneComponent*>& Siblings = AttachParent->AttachChildren;
	for (uint32 i = 0;i < Siblings.Num(); ++i)
	{
		if (Siblings[i] == this)
		{
			Siblings.RemoveAt(i, 1);
			break;
		}
	}
	AttachParent = nullptr;
}

FRotator USceneComponent::GetWorldRotation() const
{
	if (AttachParent)
	{
		FQuat ParentQuat = AttachParent->GetWorldRotation().Quaternion();
		FQuat LocalQuat = Transform.GetOrientation();

		return (ParentQuat * LocalQuat).ToFRotator();
	}

	return Transform.Rotation;
}

FVector USceneComponent::GetWorldLocation() const
{
	FMatrix WorldMatrix = GetWorldMatrix();

	return FVector(WorldMatrix[3][0], WorldMatrix[3][1], WorldMatrix[3][2]);
}

FVector USceneComponent::GetWorldScale3D() const
{
	if (AttachParent)
	{
		FVector ParentScale = AttachParent->GetWorldScale3D();

		return FVector(
			Transform.Scale.X * ParentScale.X,
			Transform.Scale.Y * ParentScale.Y,
			Transform.Scale.Z * ParentScale.Z
		);
	}

	return Transform.Scale;
}

FMatrix USceneComponent::GetWorldMatrix() const
{
	FMatrix LocalMatrix = Transform.GetLocalMatrix(); // 부모 컴포넌트 연결 없을 때

	if (AttachParent)
	{
		return LocalMatrix * AttachParent->GetWorldMatrix();
	}

	return LocalMatrix;
}

void USceneComponent::SetWorldLocation(FVector NewLocation, bool bSweep)
{
	FVector NewRelLocation = NewLocation;

	if (GetAttachParent() != nullptr)
	{		
		NewRelLocation = GetAttachParent()->GetWorldMatrix().Inverse().TransformPosition(NewRelLocation);
	}

	SetRelativeLocation(NewRelLocation);
}

void USceneComponent::SetWorldRotation(FRotator NewRotation, bool bSweep)
{
	FRotator NewRelRotation = NewRotation;

	if (GetAttachParent() != nullptr)
	{
		FQuat NewRelQuat = GetAttachParent()->GetWorldRotation().Quaternion().Inverse() * NewRelRotation.Quaternion();
		NewRelRotation = NewRelQuat.ToFRotator();
	}

	SetRelativeRotation(NewRelRotation);
}

void USceneComponent::SetWorldLocationAndRotation(FVector NewLocation, FRotator NewRotation, bool bSweep)
{
	FVector NewRelLocation = NewLocation;
	FRotator NewRelRotation = NewRotation;

	if (GetAttachParent() != nullptr)
	{
		NewRelLocation = GetAttachParent()->GetWorldMatrix().Inverse().TransformPosition(NewRelLocation);

		FQuat NewRelQuat = GetAttachParent()->GetWorldRotation().Quaternion().Inverse() * NewRelRotation.Quaternion();
		NewRelRotation = NewRelQuat.ToFRotator();
	}

	SetRelativeLocationAndRotation(NewLocation, NewRelRotation);
}

void USceneComponent::SetWorldScale3D(FVector NewScale)
{
}

void USceneComponent::SetWorldTransform(const FTransform& NewTransform, bool bSweep)
{
}

bool USceneComponent::MoveComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep)
{
	// TODO : Check Sweep, Add FHitResult
	
	SetWorldLocationAndRotation(Transform.Location + Delta, NewRotation, bSweep);

	return true;
}

void USceneComponent::MarkTransformDirty()
{
	OnTransformDirty();         

	for (USceneComponent* Child : AttachChildren)
		Child->MarkTransformDirty();      // 부모가 움직이면 자식의 월드 행렬도 바뀐다
}


