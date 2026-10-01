#include "EnginePCH.h"
#include "Engine.h"

UEngine* GEngine = nullptr;

bool UEngine::Init()
{
	World = FObjectFactory::ConstructObject<UWorld>();
	
	if (!World || !World->Init()) return false;

 	return true;
}
