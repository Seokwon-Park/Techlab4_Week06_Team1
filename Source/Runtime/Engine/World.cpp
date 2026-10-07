#include "EnginePCH.h"
#include "World.h"
#include "Level.h"

#include "Core/EngineStatics.h"
#include "GameFramework/Actor/StaticMeshActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Input/InputSystem.h"

#include "UObject/UObjectIterator.h"

#include "Collision/Ray.h"
#include "Component/BillboardComponent.h"
#include "Component/ExponentialHeightFogComponent.h"

#include "Component/StaticMeshComponent.h"
#include "Asset/LOD/StaticMeshLODSelector.h"

#include "Math/Frustum.h"

#include "Core/Stats/LightweightStats.h"
#include "Core/Stats/EditorStats.h"
#include "Core/Async/TaskPool.h"

#include "UObject/UObjectGlobals.h"

DECLARE_CYCLE_STAT("Actor Tick", STAT_ActorTick); // Actor 틱 측정
DECLARE_CYCLE_STAT("Update All Transforms", STAT_UpdateAllTransforms); // 각 Transform의 Update 시간 측정
DECLARE_CYCLE_STAT("Gather Render Packets", STAT_GatherRenderPackets);
DECLARE_CYCLE_STAT("Frustum Cull", STAT_FrustumCull);
DECLARE_CYCLE_STAT("Gather Elements", STAT_GatherElements);
DECLARE_CYCLE_STAT("Gather - LOD", STAT_GatherLOD);
DECLARE_CYCLE_STAT("Gather - Submit", STAT_GatherSubmit);

UWorld::~UWorld()
{

}

bool UWorld::Init()
{
	// Spawn Actor로 카메라 생성하고 세팅하기
	PersistentLevel = NewObject<ULevel>(this);

	if (!PersistentLevel)
	{
		HTR_LOG(Error, "Failed to create PersistentLevel");
		return false;
	}

	//레벨 연결
	PersistentLevel->SetWorld(this);
	Levels.Add(PersistentLevel);
	CurrentLevel = PersistentLevel;

	//카메라 생성
	CreateMainCamera();

	return true;
}

AActor* UWorld::SpawnActor(UClass* Class, FName InName, const FTransform* Transform)
{
	if (!Class) return nullptr;
	if (!Class->IsChildOf(AActor::StaticClass())) return nullptr;

	// 1. ObjectFactory로 Actor 생성
	AActor* NewActor = NewObject<AActor>(PersistentLevel, Class, InName);

	if (!NewActor)
	{
		HTR_LOG(Error, "SpawnActor : Failed to create Actor");
		return nullptr;
	}

	// 2. Actor에 World/Level 연결
	NewActor->World = this;
	NewActor->Level = PersistentLevel;

	// 3. Transform 적용
	const FTransform SpawnTransform = Transform ? *Transform : FTransform::Identity;

	if (NewActor->GetRootComponent())
	{
		NewActor->GetRootComponent()->SetTransform(SpawnTransform);
	}

	for (UActorComponent* Component : NewActor->GetComponents())
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
			Scene.AddPrimitive(Primitive);
		else if (UExponentialHeightFogComponent* Fog = Cast<UExponentialHeightFogComponent>(Component))
			Scene.AddFogInfo(Fog->GetUUID(), Fog->GetFogInfo());
		else if (UFireBallComponent* FireBall = Cast<UFireBallComponent>(Component))
			Scene.RegisterFireBall(FireBall);
	}

	// 4. Level->Actors에 등록
	PersistentLevel->AddActor(NewActor);

	// 5. PlayList에 추가
	BeginPlayList.Enqueue(NewActor);

	return NewActor;
}

void UWorld::Tick(float DeltaTime)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateAllTransforms);
		Scene.UpdateAllTransforms();
	}

	// 에디터 월드면 더이상 틱을 돌리지 않는다.
	if (WorldType != EWorldType::Editor)
	{
		while (!BeginPlayList.IsEmpty())
		{
			BeginPlayList.Peek()->BeginPlay();
			BeginPlayList.Dequeue();
		}

		{
			SCOPE_CYCLE_COUNTER(STAT_ActorTick);
			// 모든 Actor를 도는 대신 등록된 Tick 함수(메인 카메라 포함)만 실행한다.
			TickTaskManager.RunAllTickGroups(DeltaTime);

			for (ULevel* Level : Levels)
			{
				PathTracker.Tick(Level->GetActors(), DeltaTime);
			}
		}
	}


}

void UWorld::ClearWorld()
{
	// BeginPlay 대기 중인 Actor 제거
	while (!BeginPlayList.IsEmpty())
	{
		BeginPlayList.Dequeue();
	}

	PathTracker.SetPlaybackEnabled(false);
	PathTracker.SetPathRenderingEnabled(false);
	PathTracker.ClearPath();

	// 액터를 지우기 전에 렌더 프록시와 틱 등록부터 푼다. ClearActors는 액터를 delete만 하므로,
	// 그대로 두면 지워진 컴포넌트를 가리키는 프록시가 FScene에 남아 다음 프레임에 터진다.
	Scene.RemoveAllPrimitives();
	Scene.RemoveAllFogInfo();
	for (ULevel* Level : Levels)
	{
		for (AActor* Actor : Level->Actors)
			if (Actor)
				Actor->RegisterAllActorTickFunctions(false);
		Level->ClearActors();
	}
	HTR_LOG(Info, "{} : ", PersistentLevel->GetActorNum());
}

void UWorld::GatherRenderPackets(FRenderQueue& RenderQueue, const FLODViewContext* LODView, const FFrustumPlanes* Frustum, FRenderer* Renderer)
{
	// 멤버로 두어 매 프레임 용량을 재사용한다.
	RenderStats.Reset();
	VisibleProxies.Reset();

	{
		SCOPE_CYCLE_COUNTER(STAT_FrustumCull);
		// 컬링 단계에서는 프록시 포인터만 모으고, 컴포넌트 역참조(가시성 확인)는 어차피 컴포넌트를 읽는 Gather로 미룬다.
		const auto Visit = [&](FPrimitiveSceneProxy* Proxy) { VisibleProxies.Add(Proxy); };

		if (Frustum)
		{
			Scene.BVH.QueryCull(
				FrustumAllPlanesMask,
				[Frustum](const FBox& Bounds, uint32& Mask)
				{
					return static_cast<EBVHCullResult>(ClassifyBoxInFrustum(Bounds, *Frustum, Mask));
				},
				Visit);
		}
		else
		{
			Scene.BVH.QueryCull(0, [](const FBox&, uint32&) { return EBVHCullResult::Inside; }, Visit);
		}
	}

	RenderStats.TotalPrimitives = Scene.Proxies.Num();

	// GPU 오클루전: 프러스텀을 통과한 물체를 GPU에서 가림 판정하고 결과를 이번 프레임에 받아 온다.
	// Cull이 켜져 있으면 가려진 물체를 목록에서 빼서 이후 Gather·정렬·드로우를 모두 건너뛴다.
	// 꺼져 있으면(검증 모드) 목록은 그대로 두고 패킷에 판정만 표시한다.
	const uint8* OccludedMask = nullptr;
	if (Renderer && LODView && Renderer->GetGPUOcclusion().GetSettings().bEnabled)
	{
		FGPUOcclusion& Occlusion = Renderer->GetGPUOcclusion();
		if (Occlusion.Run(VisibleProxies.GetData(), VisibleProxies.Num(), *LODView))
		{
			const std::vector<uint8>& Occluded = Occlusion.GetOccluded();
			if (Occlusion.GetSettings().bCull)
			{
				uint32 Kept = 0;
				for (uint32 i = 0; i < VisibleProxies.Num(); ++i)
					if (!Occluded[i])
						VisibleProxies[Kept++] = VisibleProxies[i];
				VisibleProxies.SetNum(Kept);
			}
			else
			{
				OccludedMask = Occluded.data();
			}
		}
	}

	RenderStats.VisiblePrimitives = VisibleProxies.Num();

	constexpr uint32 ExtraSlots = 4096;
	const uint32 VisibleCount = VisibleProxies.Num();
	const uint32 MaxSlots = VisibleCount + ExtraSlots;
	uint8* SlotDest = Renderer ? Renderer->BeginObjectConstants(MaxSlots) : nullptr;
	uint32 NextExtraSlot = VisibleCount;

	// 조각 수 = 스레드 수 × 4. 잘게 나눠야 먼저 끝난 스레드가 남은 조각을 가져가서 부하가 고르게 된다.
	FTaskPool& Pool = FTaskPool::Get();
	const uint32 ChunkCount = FMath::Clamp(VisibleCount, 1u, Pool.GetNumThreads() * 4);

	if (GatherChunks.Num() < ChunkCount)
		GatherChunks.SetNum(ChunkCount);     // 늘릴 때만. 줄이지 않아야 배열 용량이 계속 재사용된다.

	for (uint32 c = 0; c < GatherChunks.Num(); ++c)   // 이번에 안 쓰는 조각의 묶음도 비워 둔다 (Renderer에 넘기지 않도록)
	{
		FGatherChunk& Chunk = GatherChunks[c];
		Chunk.Packets.Reset();               // 용량은 유지, 개수만 0
		Chunk.SlowPathIndices.Reset();
		std::fill(std::begin(Chunk.LODCounts), std::end(Chunk.LODCounts), 0u);
		std::fill(std::begin(Chunk.LODTriangles), std::end(Chunk.LODTriangles), 0ull);
		for (FStaticDrawGroup& Group : Chunk.Groups)
			Group.Items.clear();             // 용량은 유지
		Chunk.StaticDrawCount = 0;
	}

	// Renderer가 있으면 불투명 스태틱 메시는 패킷 대신 조각별 묶음에 작은 항목으로 넣는다.
	// 동일 바인딩의 개별 드로우를 묶어 패킷 복사와 항목별 정렬을 줄인다.
	const bool bStaticGroups = Renderer != nullptr;
	if (Renderer)
		Renderer->ResetStaticDrawGroups();

	{
		SCOPE_CYCLE_COUNTER(STAT_GatherElements);

		LODInputs.Reset();
		if (LODView)
		{
			LODInputs.Reserve(VisibleProxies.Num());
			for (const FPrimitiveSceneProxy* Proxy : VisibleProxies)
				LODInputs.Add({ Proxy->GetLODSphere(), Proxy->GetRenderState() });
			SelectLODs(LODInputs, *LODView, SelectedLODs);
		}
		Pool.ParallelFor(VisibleCount, ChunkCount, [&](uint32 Begin, uint32 End, uint32 ChunkIndex)
			{
				FGatherChunk& Out = GatherChunks[ChunkIndex];      // 이 조각 전용. 다른 스레드는 절대 안 건드림
				// (머티리얼, 메시, LOD) 묶음 찾기. 조합이 몇 개뿐이라 선형 탐색이면 충분하고, 바로 전 묶음을 먼저 본다.
				const auto FindGroup = [&Out](UMaterial* Material, UStaticMesh* Mesh, uint8 LOD) -> FStaticDrawGroup&
					{
						if (Out.LastGroup < Out.Groups.size())
						{
							FStaticDrawGroup& Last = Out.Groups[Out.LastGroup];
							if (Last.Material == Material && Last.Mesh == Mesh && Last.LODIndex == LOD)
								return Last;
						}
						for (uint32 g = 0; g < Out.Groups.size(); ++g)
						{
							FStaticDrawGroup& Group = Out.Groups[g];
							if (Group.Material == Material && Group.Mesh == Mesh && Group.LODIndex == LOD)
							{
								Out.LastGroup = g;
								return Group;
							}
						}
						Out.LastGroup = static_cast<uint32>(Out.Groups.size());
						FStaticDrawGroup& Group = Out.Groups.emplace_back();
						Group.Material = Material;
						Group.Mesh = Mesh;
						Group.LODIndex = LOD;
						return Group;
					};

				for (uint32 VisibleIndex = Begin; VisibleIndex < End; ++VisibleIndex)
				{
					FPrimitiveSceneProxy* Proxy = VisibleProxies[VisibleIndex];   // 읽기만
					if (!Proxy->IsVisible()) continue;

					UStaticMesh* Mesh = Proxy->GetMesh();
					if (!Mesh)
					{
						Out.SlowPathIndices.Add(VisibleIndex);     // 컴포넌트를 건드리는 경로는 메인이 나중에
						continue;
					}

					const uint32 LOD = LODView ? SelectedLODs[VisibleIndex] : 0;
					const FCachedMeshLOD& CachedLOD = Proxy->GetLOD(LOD);
					++Out.LODCounts[LOD];                          // RenderStats 대신 조각 전용 통계

					uint32 Slot = InvalidObjectSlot;
					if (SlotDest)
					{
						// 칸 VisibleIndex는 이 반복만 쓴다 → 스레드끼리 겹치지 않음
						std::memcpy(SlotDest + size_t(VisibleIndex) * ObjectSlotBytes,
							&Proxy->GetLocalToWorld(), sizeof(FMatrix));
						Slot = VisibleIndex;
					}

					const bool bOccludedByGpu = OccludedMask && OccludedMask[VisibleIndex];
					for (uint32 i = 0; i < CachedLOD.NumSections; ++i)
					{
						const FCachedMeshSection& Section = Proxy->GetSection(CachedLOD.FirstSection + i);
						Out.LODTriangles[LOD] += Section.IndexCount / 3;

						if (bStaticGroups && Section.Material && Section.Material->BlendState == EBlendState::Opaque)
						{
							FStaticDrawGroup& Group = FindGroup(Section.Material, Mesh, static_cast<uint8>(LOD));
							Group.Items.push_back({ Proxy, Slot, Section.StartIndex, Section.IndexCount, bOccludedByGpu ? 1u : 0u });
							++Out.StaticDrawCount;
							continue;
						}

						FRenderPacket& Packet = Out.Packets.AddDefaulted_GetRef();   // 조각 전용 배열에 추가
						Packet.Proxy = Proxy;
						Packet.Mesh = Mesh;
						Packet.Material = Section.Material;
						Packet.StartIndex = Section.StartIndex;
						Packet.IndexCount = Section.IndexCount;
						Packet.LODIndex = static_cast<uint8>(LOD);
						Packet.Slot = Slot;
						Packet.bOccludedByGpu = bOccludedByGpu;
					}
				}
			});

		// 묶음은 World 메모리 그대로 Renderer에 넘긴다 (복사 없음). 정렬은 Renderer가 묶음 단위로 한다.
		uint32 StaticDraws = 0;
		if (Renderer)
		{
			for (uint32 c = 0; c < ChunkCount; ++c)
			{
				StaticDraws += GatherChunks[c].StaticDrawCount;
				for (const FStaticDrawGroup& Group : GatherChunks[c].Groups)
					if (!Group.Items.empty())
						Renderer->AddStaticDrawGroup(&Group);
			}
		}

		uint32 TotalPackets = 0;
		for (uint32 c = 0; c < ChunkCount; ++c)
			TotalPackets += GatherChunks[c].Packets.Num();

		RenderQueue.Reserve(TotalPackets + 256);             // 느린 경로 몫 약간 여유
		for (uint32 c = 0; c < ChunkCount; ++c)
			if (GatherChunks[c].Packets.Num() > 0)
				RenderQueue.Append(GatherChunks[c].Packets);  // 조각 순서대로 이어 붙이기

		for (uint32 c = 0; c < ChunkCount; ++c)
		{
			for (uint32 VisibleIndex : GatherChunks[c].SlowPathIndices)
			{
				FPrimitiveSceneProxy* Proxy = VisibleProxies[VisibleIndex];
				UPrimitiveComponent* Primitive = Proxy->GetComponent();
				if (!Primitive || !Primitive->IsVisible())
					continue;

				const uint32 FirstNew = RenderQueue.Num();

				// 프록시 캐시가 없는 스태틱 메시는 기존처럼 LOD를 골라 제출하고, 그 외는 컴포넌트에 맡긴다.
				UStaticMeshComponent* StaticMeshComponent = LODView ? Cast<UStaticMeshComponent>(Primitive) : nullptr;
				if (StaticMeshComponent && StaticMeshComponent->GetStaticMesh())
				{
					const uint32 LOD = SelectedLODs[VisibleIndex];
					StaticMeshComponent->SubmitToRenderQueue(RenderQueue, LOD);
				}
				else
				{
					Primitive->SubmitToRenderQueue(RenderQueue);
				}

				// continue 없이 항상 여기까지 와서 새 패킷에 여유 칸을 배정한다.
				for (uint32 p = FirstNew; p < RenderQueue.Num(); ++p)
				{
					if (!SlotDest || NextExtraSlot >= MaxSlots) break;
					FRenderPacket& Packet = RenderQueue[p];
					const FMatrix& Model = Packet.Proxy ? Packet.Proxy->GetLocalToWorld() : Packet.Model ? *Packet.Model : FMatrix::Identity;
					std::memcpy(SlotDest + size_t(NextExtraSlot) * ObjectSlotBytes, &Model, sizeof(FMatrix));
					Packet.Slot = NextExtraSlot++;
				}
			}
		}

		for (uint32 c = 0; c < ChunkCount; ++c)
			for (uint32 L = 0; L < 4; ++L)
			{
				RenderStats.LODCounts[L] += GatherChunks[c].LODCounts[L];
				RenderStats.LODTriangles[L] += GatherChunks[c].LODTriangles[L];
			}

		if (SlotDest) Renderer->EndObjectConstants();
		RenderStats.DrawCalls = RenderQueue.Num() + StaticDraws;
		for (uint64 T : RenderStats.LODTriangles) RenderStats.Triangles += T;
	}
}

//void UWorld::GatherRenderPackets(TArray<FRenderPacket>& RenderArray, const FLODViewContext* LODView, const FFrustumPlanes* Frustum)
//{
//	for (TObjectIterator<UPrimitiveComponent> Itr; Itr; ++Itr)
//	{
//		if (!*Itr || !Itr->IsVisible())
//			continue;
//
//		if (Frustum)
//		{
//			const FBox Box = Itr->CalcBounds();
//			const FAABB Bounds{
//				(Box.Min + Box.Max) * 0.5f,
//				(Box.Max - Box.Min) * 0.5f
//			};
//			if (!IsAABBInFrustum(Bounds, *Frustum))
//				continue;
//		}
//
//		if (LODView)
//		{
//			if (auto* Component = Cast<UStaticMeshComponent>(*Itr))
//			{
//				if (UStaticMesh* Mesh =
//					Component->GetStaticMesh())
//				{
//					const uint32 LOD = SelectStaticMeshLOD(
//						*Mesh,
//						Component->GetWorldMatrix(),
//						*LODView);
//
//					Component->SubmitToRenderQueue(RenderQueue, LOD);
//					continue;
//				}
//			}
//		}
//
//		Itr->SubmitToRenderQueue(RenderQueue);
//	}
//}

// 메인 카메라 생성
void UWorld::CreateMainCamera()
{
	if (MainCamera)
		return;

	MainCamera = NewObject<ACameraActor>();

	if (!MainCamera)
	{
		HTR_LOG(Error, "Failed to create MainCamera");
		return;
	}

	MainCamera->World = this;
	MainCamera->Level = nullptr;
	MainCamera->GetCameraComponent()->SetRelativeLocation(FVector(-5.0f, -5.0f, 5.0f));

	// 메인 카메라는 Level에 속하지 않아 BeginPlay를 거치지 않으므로 여기서 등록한다.
	MainCamera->RegisterAllActorTickFunctions(true);
}

void UWorld::SetMainCamera(ACameraActor* Camera)
{
	if (MainCamera == Camera)
		return;

	if (MainCamera)
		MainCamera->RegisterAllActorTickFunctions(false);

	MainCamera = Camera;

	if (MainCamera)
	{
		MainCamera->World = this;
		MainCamera->RegisterAllActorTickFunctions(true);
	}
}

int32 UWorld::GetActorNum()
{
	return PersistentLevel->GetActorNum();
}

bool UWorld::DestroyActor(AActor* Actor)
{
	if (!Actor)
		return false;

	ULevel* Level = Actor->GetLevel();

	if (!Level)
		return false;

	// 1. BeginPlay 대기열에서 제거
	TQueue<AActor*> NewBeginPlayList;

	while (!BeginPlayList.IsEmpty())
	{
		AActor* PendingActor = BeginPlayList.Peek();
		BeginPlayList.Dequeue();

		if (PendingActor != Actor)
		{
			NewBeginPlayList.Enqueue(PendingActor);
		}
	}

	BeginPlayList = std::move(NewBeginPlayList);

	// 2. PathTracker에서 제거
	PathTracker.OnObjectDestroyed(Actor);

	//// 3. PrimitiveComponents에서 제거
	//for (UActorComponent* Component : Actor->Components)
	//{
	//	UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);

	//	if (!Primitive)
	//		continue;

	//	for (int32 i = PrimitiveComponents.Num() - 1; i >= 0; --i)
	//	{
	//		if (PrimitiveComponents[i] == Primitive)
	//		{
	//			PrimitiveComponents.RemoveAt(i, 1);
	//			break;
	//		}
	//	}
	//}

	// 4. Level의 Actors에서 제거
	for (int32 i = Level->Actors.Num() - 1; i >= 0; --i)
	{
		if (Level->Actors[i] == Actor)
		{
			Level->Actors.RemoveAt(i, 1);
			break;
		}
	}

	FString ActorName = Actor->GetName();
	uint32 ActorUUID = Actor->GetUUID();

	// 5. 프록시 제거
	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
		{
			Scene.RemovePrimitive(Primitive);
		}
		else if (UExponentialHeightFogComponent* Fog = Cast<UExponentialHeightFogComponent>(Component))
		{
			Scene.RemoveFogInfo(Fog->GetUUID());
		}
	}

	Actor->RegisterAllActorTickFunctions(false);

	// 6. Actor 삭제
	delete Actor;

	HTR_LOG(Info, "Destroy Actor : {} UUID {}", ActorName, ActorUUID);

	return true;
}

// 다른 World의 객체를 제외하고 Component 교차 중 최근접 결과를 선택한다.
bool UWorld::LineTraceSingle(const FRay& WorldRay, FHitResult& OutHit,
	FBillboardTraceTransform ResolveBillboard, const void* ViewContext)
{
	SCOPE_CYCLE_COUNTER_ALWAYS(EditorStats::STAT_PickingTime_Name);
	OutHit = FHitResult();
	float NearestT = std::numeric_limits<float>::max();

	const auto TraceComponent = [&](FPrimitiveSceneProxy* Proxy, float& InOutNearestT)
		{
			if (UStaticMesh* Mesh = Proxy ? Proxy->GetMesh() : nullptr)
			{
				if (!Proxy->IsVisible())
					return false;

				const FMatrix& WorldToLocal = Proxy->GetWorldToLocal();
				const FRay LocalRay{
					.Origin = WorldToLocal.TransformPosition(WorldRay.Origin),
					.Direction = WorldToLocal.TransformVector(WorldRay.Direction)
				};

				float T = InOutNearestT;
				if (!RayIntersectsMesh(LocalRay, Mesh->GetMeshData(), T))
					return false;

				OutHit.HitComponent = Proxy->GetComponent();
				OutHit.Distance = T;
				OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
				InOutNearestT = T;
				return true;
			}

			UPrimitiveComponent* Component = Proxy ? Proxy->GetComponent() : nullptr;

			if (!Component || !Component->IsVisible())
				return false;

			if (UBillboardComponent* Billboard = Cast<UBillboardComponent>(Component))
			{
				if (!ResolveBillboard)
				{
					FHitResult Hit;
					if (!Billboard->LineTraceComponent(WorldRay, Hit) ||
						Hit.Distance >= InOutNearestT)
					{
						return false;
					}

					OutHit = Hit;
					InOutNearestT = Hit.Distance;
					return true;
				}

				const FMatrix BillboardToWorld = ResolveBillboard(*Billboard, ViewContext);

				const FRay LocalRay = ToLocalRay(WorldRay, BillboardToWorld);

				float T = InOutNearestT;
				if (!Billboard->LineTraceComponentLocal(LocalRay, T))
				{
					return false;
				}

				OutHit.HitComponent = Billboard;
				OutHit.Distance = T;
				OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
				InOutNearestT = T;
				return true;
			}

			const FMatrix& WorldToLocal = Proxy->GetWorldToLocal();
			const FRay LocalRay{
				.Origin = WorldToLocal.TransformPosition(WorldRay.Origin),
				.Direction = WorldToLocal.TransformVector(WorldRay.Direction)
			};

			float T = InOutNearestT;
			if (!Component->LineTraceComponentLocal(LocalRay, T))
			{
				return false;
			}

			OutHit.HitComponent = Component;
			OutHit.Distance = T;
			OutHit.ImpactPoint = WorldRay.Origin + WorldRay.Direction * T;
			InOutNearestT = T;
			return true;
		};

	const FPreparedRay PreparedRay(WorldRay);

	Scene.BVH.TraceClosest(
		[&](const FBox& Bounds, float& OutEnterT) { return RayIntersectsAABB(PreparedRay, Bounds.Min, Bounds.Max, OutEnterT); },
		[&](FPrimitiveSceneProxy* Proxy, float& OutNearestT) { return TraceComponent(Proxy, OutNearestT); },
		NearestT);

	return OutHit.HitComponent != nullptr;
}

void UWorld::BeginPlay()
{
}

void UWorld::EndPlay()
{
}

UWorld* UWorld::CreateWorld(const EWorldType InWorldType, bool bInformEngineOfWorld, FName WorldName)
{
	UWorld* NewWorld = NewObject<UWorld>(nullptr, WorldName);
	NewWorld->SetFlags(EObjectFlags::RF_Transactional);
	NewWorld->WorldType = InWorldType;
	NewWorld->Init();

	return NewWorld;
}

UWorld* UWorld::GetDuplicatedWorldForPIE(UWorld * InWorld)
{
	if (!InWorld)
	{
		return nullptr;
	}

	UWorld* PIEWorld = CreateWorld(EWorldType::PIE, false);

	FObjectDuplicationParameters Params(InWorld, nullptr);
	Params.DuplicationSeed.Add(InWorld, PIEWorld);
	Params.DuplicationSeed.Add(InWorld->GetPersistentLevel(), PIEWorld->GetPersistentLevel());
	Params.PortFlags = EPropertyPortFlags::PPF_DuplicateForPIE;

	StaticDuplicateObjectEx(Params);
	return PIEWorld;
}

void UWorld::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	// 아직 필요 없는애들
	//TArray<UObject*> ObjectsToFixReferences;
	//TMap<UObject*, UObject*> ReplacementMap;

	if (!bDuplicateForPIE)
	{
		assert(PersistentLevel);

		// Update the persistent level's owning world. This is needed for some initialization
		if (!PersistentLevel->OwningWorld)
		{
			PersistentLevel->OwningWorld = this;
		}
	}

	for (AActor* Actor : PersistentLevel->GetActors())
	{
		if (!Actor)
			continue;

		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
				SceneComp->MarkTransformDirty();        // 부착/상대 Transform이 바뀌었으니 다시 계산

			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
				Scene.AddPrimitive(Primitive);
			else if (UExponentialHeightFogComponent* Fog = Cast<UExponentialHeightFogComponent>(Component))
				Scene.AddFogInfo(Fog->GetUUID(), Fog->GetFogInfo());
			else if (UFireBallComponent* FireBall = Cast<UFireBallComponent>(Component))
				Scene.RegisterFireBall(FireBall);
		}

		BeginPlayList.Enqueue(Actor);   // Tick 등록은 BeginPlay에서 함
	}

	Scene.BuildBVH();
}

void UWorld::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);   // "Properties"

	FArchive& Ar = Record.GetUnderlyingArchive();
	PersistentLevel->Serialize(Record.EnterRecord("PersistentLevel"));

	// (선택) 메인 카메라는 레벨에 속하지 않아 따로 쓴다. PIE 시작 시점이나 에디터 시점을 복원할 때 쓸 수 있다.
	// MainCamera->GetCameraComponent()->Serialize(Record.EnterRecord("MainCamera"));
}
