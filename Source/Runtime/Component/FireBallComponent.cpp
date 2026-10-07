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
            World->GetScene().RegisterFireBall(this);
        }
    }
}

UFireBallComponent::~UFireBallComponent()
{
    if (AActor* Owner = GetOwner())
    {
        if (UWorld* World = Owner->GetWorld())
        {
            World->GetScene().UnregisterFireBall(this);
        }
    }
}