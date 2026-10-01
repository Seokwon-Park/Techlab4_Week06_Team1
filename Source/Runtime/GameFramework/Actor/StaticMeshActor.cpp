#include "EnginePCH.h"
#include "StaticMeshActor.h"

#include "Component/PrimitiveComponent.h"
#include "ObjectSystem/ObjectFactory.h"
#include "Asset/AssetManager.h"

AStaticMeshActor::AStaticMeshActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("UPrimitiveComponent");
	SetRootComponent(StaticMeshComponent);

}

void AStaticMeshActor::SetPrimitiveType(EPrimitiveType Type)
{

}


void AStaticMeshActor::BeginPlay()
{
	Super::BeginPlay();
}
