#include "EnginePCH.h"
#include "Actor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Component/SceneComponent.h"
#include "Component/ParticleSubUVComponent.h"
#include "Core/EngineLog.h"

enum class EAttachmentRule;


AActor::AActor()
{
    PrimaryActorTick.Target = this;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>("DefaultSceneRoot"));
}

AActor::~AActor()
{
    TArray<UActorComponent*> ToDelete = Components;
    Components.Reset();
    RootComponent = nullptr;

    for (UActorComponent* Component : ToDelete)
    {
        delete Component;
    }
}

void AActor::BeginPlay()
{
	//if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(RootComponent))
	//{
	//	World->AddPrimitive(Cast<UPrimitiveComponent>(RootComponent));
	//}

	for (UActorComponent* Component : Components)
	{
		Component->BeginPlay();
	}

	RegisterAllActorTickFunctions(true);
}

void AActor::RegisterAllActorTickFunctions(bool bRegister)
{
	if (bRegister && !World)
		return;

	// bCanEverTick이 꺼진 함수는 등록하지 않으므로 정적 메시 액터는 매 프레임 순회 대상에서 빠진다.
	auto Apply = [&](FTickFunction& Function)
	{
		if (bRegister)
			Function.RegisterTickFunction(World->GetTickTaskManager());
		else
			Function.UnRegisterTickFunction();
	};

	Apply(PrimaryActorTick);
	for (UActorComponent* Component : Components)
	{
		if (Component)
			Apply(Component->PrimaryComponentTick);
	}
}

void AActor::RemoveOwnedComponent(UActorComponent* Component)
{
    for (uint32 i = 0; i < Components.Num(); ++i)
    {
        if (Components[i] == Component)
        {
            Components.RemoveAt(i, 1);
            break;
        }
    }

    if (RootComponent == Component)
    {
        RootComponent = nullptr;
    }
}

FVector AActor::GetActorLocation() const
{
    if (RootComponent)
    {
        return RootComponent->GetWorldLocation();
    }
    return FVector::ZeroVector;
}

//FRotator AActor::GetActorRotation() const
//{
//    if (RootComponent)
//    {
//        // USceneComponent의 GetWorldRotation() 호출
//        return RootComponent->GetWorldRotation();
//    }
//    return FRotator::ZeroRotator;
//}

FVector AActor::GetActorScale3D() const
{
    if (RootComponent)
    {
        return RootComponent->GetWorldScale3D();
    }
    return FVector::OneVector;
}

//FQuat AActor::GetActorQuat() const
//{
//    if (RootComponent)
//    {
//        return FQuat(RootComponent->GetWorldRotation());
//    }
//    return FQuat::Identity;
//}

FTransform AActor::GetActorTransform() const
{
    if (RootComponent)
    {
        return FTransform(
            RootComponent->GetWorldRotation(),
            RootComponent->GetWorldLocation(),
            RootComponent->GetWorldScale3D()
        );

        // return FTransform(RootComponent->GetWorldMatrix());
    }
    return FTransform::Identity;
}

bool AActor::Destroy()
{
    if (!World)
        return false;

    return World->DestroyActor(this);
}

UActorComponent* AActor::AddComponent(UClass* ComponentClass, FName Name)
{
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return nullptr;
    }

    UActorComponent* NewComponent = NewObject<UActorComponent>(this, ComponentClass, Name);
    if (!NewComponent)
    {
        return nullptr;
    }

	if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(NewComponent))
	{
        World->GetScene().AddPrimitive(Primitive);
	}
    
	if (USceneComponent* SceneComponent = Cast<USceneComponent>(NewComponent))
	{
		SceneComponent->SetupAttachment(RootComponent, EAttachmentRule::KeepRelative);
	}

    NewComponent->SetOwner(this);
    Components.Add(NewComponent);

    if (UParticleSubUVComponent* ParticleSubUV = Cast<UParticleSubUVComponent>(NewComponent))
    {
        ParticleSubUV->BeginPlay();
        RegisterAllActorTickFunctions(true);
    }

	return NewComponent;
}

void AActor::DestroyComponent(UActorComponent* Component)
{
    if (!Component)
        return;

    if (Component == RootComponent)
    {
        HTR_LOG(Warning, "DefaultSceneRoot cannot be deleted.");
        return;
    }

    if (USceneComponent* Parent = Cast<USceneComponent>(Component))
    {
        TArray<USceneComponent*> Children = Parent->GetAttachChildren();
        for (USceneComponent* Candidate : Children)
        {
            if (!Candidate) { continue; }

            Candidate->DetachFromParent(EAttachmentRule::KeepWorld);
            Candidate->SetupAttachment(Parent->GetAttachParent(), EAttachmentRule::KeepWorld);
        }
    }

    if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
    {
        World->GetScene().RemovePrimitive(Primitive);
    }
    RemoveOwnedComponent(Component);
    delete Component;
    return;
}