#include "EnginePCH.h"
#include "Level.h"
#include "World.h"

// delete Actor가 소멸자를 호출하려면 AActor의 완전한 정의가 필요하다.
// 전방 선언만으로는 C4150(소멸자 미호출)이 되어 액터가 GUObjectArray에 남는다.
#include "GameFramework/Actor.h"
#include "Component/ExponentialHeightFogComponent.h"

void ULevel::AddActor(AActor* Actor)
{
	if (!Actor)
		return;

	Actors.Add(Actor);
}

void ULevel::ClearActors()
{
	for (AActor* Actor : Actors)
	{
		if (!Actor)
			continue;

		delete Actor;
	}

	Actors.Reset();
}

void ULevel::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	FArchive& Ar = Record.GetUnderlyingArchive();

	if (Ar.HasAnyPortFlags(EPropertyPortFlags::PPF_Duplicate))
	{
		int32 Num = Actors.Num();
		FStructuredArchive::FArray ActorArray = Record.EnterArray("Actors", Num);
		if (Ar.IsLoading())
			Actors.SetNum(Num);
		for (int32 i = 0; i < Num; ++i)
		{
			UObject* Obj = Actors[i];
			ActorArray.EnterElement() << Obj;
			Actors[i] = static_cast<AActor*>(Obj);
		}

		UObject* WorldObj = OwningWorld;
		Record << SA_VALUE("OwningWorld", WorldObj);
		OwningWorld = static_cast<UWorld*>(WorldObj);
		return;
	}

	if (Ar.IsSaving())
	{
		// 개수를 먼저 알아야 EnterArray를 할 수 있으므로 저장할 액터부터 고른다
		TArray<AActor*> SavedActors;
		for (AActor* Actor : Actors)
		{
			if (Actor && !Actor->HasAnyFlags(EObjectFlags::RF_Transient))
				SavedActors.Add(Actor);
		}

		int32 Num = SavedActors.Num();
		FStructuredArchive::FArray ActorArray = Record.EnterArray("Actors", Num);
		for (AActor* Actor : SavedActors)
		{
			FStructuredArchive::FRecord ActorRecord = ActorArray.EnterElement().EnterRecord();
			FString ClassName = Actor->GetClass()->Name;
			ActorRecord << SA_VALUE("Class", ClassName);
			Actor->Serialize(ActorRecord);
		}
		return;
	}

	// 불러오기: 월드를 비우는 건 호출하는 쪽 책임 (PIE 복제는 새 월드라 비울 필요가 없다)
	assert(OwningWorld);

	int32 Num = 0;
	FStructuredArchive::FArray ActorArray = Record.EnterArray("Actors", Num);
	for (int32 i = 0; i < Num; ++i)
	{
		FStructuredArchive::FRecord ActorRecord = ActorArray.EnterElement().EnterRecord();

		FString ClassName;
		ActorRecord << SA_VALUE("Class", ClassName);

		UClass* Class = FindClass(ClassName);
		if (!Class || !Class->IsChildOf(AActor::StaticClass()))
		{
			HTR_LOG(Warning, "Load: unknown actor class '{}', skipped", ClassName);
			continue;
		}

		if (AActor* Actor = OwningWorld->SpawnActor(Class))
		{
			Actor->Serialize(ActorRecord);

			// SpawnActor가 기본값으로 등록한 Fog를 방금 읽은 값으로 갱신한다.
			for (UActorComponent* Component : Actor->GetComponents())
				if (UExponentialHeightFogComponent* Fog = Cast<UExponentialHeightFogComponent>(Component))
					OwningWorld->GetScene().UpdateFogInfo(Fog->GetUUID(), Fog->GetFogInfo());
		}
	}
}
