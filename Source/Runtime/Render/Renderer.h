#pragma once

#include "RenderPacket.h"
#include "Texture2D.h"
#include "Text/Font.h"

#include "RenderingInfo.h"
#include "Occlusion/GPUOcclusion.h"

constexpr uint32 ObjectSlotBytes = 256;
constexpr uint32 MaxFireBalls = 16;

struct FPerObjectConstants
{
	FMatrix World;
};

struct FSortEntry
{
	uint64 Key;
	uint32 PacketIndex;
};

class UCameraComponent;

// 오클루전 컬링의 효과 상한을 재기 위한 측정 결과 (디버그 전용)
struct FOcclusionMeasureResult
{
	bool bValid = false;
	uint32 TotalObjects = 0;      // 불투명 패스에 제출된 물체 수
	uint32 VisibleObjects = 0;    // 최종 깊이 버퍼에 픽셀을 1개 이상 남긴 물체 수
	uint32 TotalDraws = 0;
	uint32 VisibleDraws = 0;
	uint64 TotalTriangles = 0;
	uint64 VisibleTriangles = 0;
	double ElapsedMs = 0.0;       // 측정 자체에 걸린 시간 (GPU 대기 포함)
	// GPU 오클루전 검증. 판정만 하고 거르지 않은(Cull 끔) 상태에서 측정해야 의미가 있다.
	uint32 GPUOccludedDraws = 0;  // GPU가 가렸다고 판정한 드로우 수
	uint32 FalseCulls = 0;        // 그중 실제로는 픽셀이 보인 드로우 수. 0이어야 한다
};

struct alignas(16) FFireBallLight
{
	FVector4 PositionRadius; // xyz: 위치, w: 반경
	FVector4 ColorIntensity; // rgb: 색상, w: 강도
	FVector4 FalloffEnabled; // x: 반경 감쇠, y: 사용 여부, zw: 패딩
};

struct alignas(16) FFireBallLightConstants
{
	FFireBallLight Light[MaxFireBalls];
	uint32 LightCount;
	uint32 Padding[3] = {};
};

class FRenderer
{
public:
	bool Init();

	// 기존 단일 카메라의 ViewProjection으로 렌더 큐 전체를 그린다.
	void RenderAll(FRenderQueue& InQueue, UCameraComponent* CameraComponent);

	// Adapter가 계산한 ViewProjection을 직접 받아 View별 렌더 큐를 그린다.
	void RenderAll(FRenderQueue& InQueue, const FMatrix& ViewProjection);

	// 큐를 정렬해 불투명 패킷만 그린다. 반투명은 RenderTranslucent 호출 전까지 보관한다.
	void RenderOpaque(const FMatrix& ViewProjection);

	// Gather 전에 World가 불러 가려진 물체를 거른다.
	FGPUOcclusion& GetGPUOcclusion() { return GPUOcclusion; }

	// 캐시된 스태틱 메시 경로: World::Gather가 조각별로 채운 묶음을 이번 프레임 불투명 패스에 넘긴다.
	// 묶음 메모리는 World 소유이며 다음 Gather 전까지 유효하다. RenderQueueSorting이 묶음을 정렬하고 RenderOpaque가 그린다.
	void ResetStaticDrawGroups() { StaticGroups.clear(); }
	void AddStaticDrawGroup(const FStaticDrawGroup* Group) { StaticGroups.push_back(Group); }

	// RenderOpaque가 보관한 반투명 패킷을 먼 것부터 그린다.
	void RenderTranslucent(const FMatrix& ViewProjection);

	// 큐를 Material, Opaque, Mesh에 따라 정렬한다
	void RenderQueueSorting(FRenderQueue& InQueue, const FMatrix& ViewProjection);

	// [측정 전용] RenderOpaque 직후에 호출한다. 불투명 패킷을 깊이 LESS_EQUAL·색 쓰기 없이 다시 그리며
	// 패킷마다 오클루전 쿼리를 걸어, 최종 화면에 실제로 픽셀을 남긴 물체 수를 센다.
	// GPU 결과를 기다리므로 매우 느리다. 버튼 등으로 한 프레임만 실행할 것.
	FOcclusionMeasureResult MeasureOpaqueOcclusion(const FMatrix& ViewProjection);

	uint8* BeginObjectConstants(uint32 MaxSlots);
	void EndObjectConstants();

	void SetFireBallLight(const FFireBallLightConstants& LightConstants);

private:
	// FIFO 소비용 배열의 용량만 재사용하며 매 View의 패킷 값은 새로 채운다.
	FRenderQueue RenderPackets;
	// 정렬된 RenderPackets에서 반투명 패킷이 시작되는 위치
	uint32 FirstTranslucentIndex = 0;
	uint32 FirstMaterialIndex = 0;
	TUniquePtr<FConstantBuffer> PerObjectCB;
	TUniquePtr<FConstantBuffer> ViewCB;

	// 모든 패킷의 World 행렬을 256바이트 칸에 한 번에 올린 버퍼. D3D11.1 오프셋 바인딩을 못 쓰면 PerObjectCB로 돌아간다.
	TUniquePtr<FConstantBuffer> PerObjectSlotCB;
	uint32 PerObjectSlotCapacity = 0;
	bool bUsePerObjectSlots = false;
	bool bObjectConstantsPrepared = false;
	UMaterial* LastMaterial;
	UStaticMesh* LastMesh;

	// MeasureOpaqueOcclusion 전용. 처음 측정할 때 만들고 이후 재사용한다.
	TArray<ComPtr<ID3D11Query>> OcclusionQueries;
	ComPtr<ID3D11DepthStencilState> DepthLessEqualReadOnly;

	FGPUOcclusion GPUOcclusion;

	void DrawPackets(uint32 Begin, uint32 End, const FMatrix& ViewProjection);
	void DrawStaticGroups();
	void UpdatePerObjectConstants(const FMatrix& World);

	// 이번 프레임 스태틱 메시 묶음 (정렬 키 순). 비어 있지 않은 것만 담는다.
	std::vector<const FStaticDrawGroup*> StaticGroups;
	void BindMaterial(UMaterial* material);
	void UpdateMaterialParams(const FRenderPacket& RenderPacket);
	void UpdatePerObjectConstants(const FRenderPacket& RenderPacket, const FMatrix& ViewProjection);
	void EnsurePerObjectSlotCapacity(uint32 SlotCount);
	void UploadPerObjectConstants();

	TArray<FSortEntry> SortEntries;

	TUniquePtr<FConstantBuffer> FireBallLightCB;
	FFireBallLightConstants FireBallLightData{};
};
