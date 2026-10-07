#include "EnginePCH.h"
#include "FireBallComponent.h"
#include "Engine/World.h"

void UFireBallComponent::InitializeComponent()
{
    Super::InitializeComponent();

    if (AActor* Owner = GetOwner())
    {
        if (UWorld* World = Owner->GetWorld())
        {
            World->RegisterFireBall(this);
        }
    }
}

UFireBallComponent::~UFireBallComponent()
{
    if (AActor* Owner = GetOwner())
    {
        if (UWorld* World = Owner->GetWorld())
        {
            World->UnregisterFireBall(this);
        }
    }
}