#pragma once

#include "Editor/LevelEditor/MultipleViewports/Adapter/MultipleViewportsAdapterTypes.h"

#include "Collision/Ray.h"
#include "Container/Queue.h"
#include "Math/Matrix.h"
#include "Render/RenderPacket.h"
#include "Asset/LOD/StaticMeshLODSelector.h"

#include "Container/Map.h"
#include "Container/Array.h"

class FOutlinerPanel;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UWorld;

// 팀 엔진 데이터와 MultipleViewports Core API 사이의 상태·변환·렌더 연결을 맡는다.
class FMultipleViewportsAdapter
{
public:
    // 엔진 메인 카메라 설정을 읽고 네 View의 초기 상태와 프리셋을 구성한다.
    void InitializeFromWorld(UWorld& World);

    // 현재 Single·Quad 레이아웃 모드를 설정한다.
    void SetLayoutMode(ELayoutMode Mode) { Views.Mode = Mode; }
    // 현재 레이아웃 모드를 반환한다.
    ELayoutMode GetLayoutMode() const { return Views.Mode; }
    // Single 레이아웃에서 전체 화면으로 표시할 View 인덱스를 설정한다.
    void SetSingleViewIndex(int32 ViewIndex);
    // Single 레이아웃에서 선택된 View 인덱스를 반환한다.
    int32 GetSingleViewIndex() const { return SingleViewIndex; }
    // 외부 카메라 값을 지정 View에 복사하고 투영 모드에 맞는 프리셋을 설정한다.
    void SetViewCamera(int32 ViewIndex, const FViewCamera& Camera);
    // 속성 창의 변경을 반영하되 위치·투영 수치만 바뀌면 기존 축 정렬 프리셋을 유지한다.
    void ApplyCameraProperties(int32 ViewIndex, const FViewCamera& Camera, bool bRotationChanged);
    // 지정 View의 Native 카메라 상태를 반환한다.
    const FViewCamera& GetViewCamera(int32 ViewIndex) const;
    // 선택한 프리셋의 위치·방향·투영 모드를 지정 View에 적용한다.
    void ApplyCameraPreset(int32 ViewIndex, EMultipleViewportsCameraPreset Preset);
    // 지정 View에 적용된 카메라 프리셋을 반환한다.
    EMultipleViewportsCameraPreset GetCameraPreset(int32 ViewIndex) const;
    // 프리셋 또는 카메라 Forward 지배축으로 표시할 Grid 평면을 고른다.
    EGridPlane GetGridPlane(int32 ViewIndex) const;

    // 레이아웃 Rect를 계산하고 마우스 위치로 Hover·활성 View를 갱신한다.
    void UpdateLayout(FVector2 WindowSize, FVector2 LocalMousePosition);
    // Splitter Drag 픽셀을 Core 비율 계산에 전달해 레이아웃 상태를 갱신한다.
    void ApplySplitterDrag(EDragAxis Axis, float DeltaPixels, FVector2 WindowSize);
    // 우클릭 Capture View에 이동·Euler Yaw/Pitch·줌 입력을 적용한다.
    void UpdateInput(float DeltaTime, FVector2 LocalMousePosition, float MoveSpeed, float MouseSensitivity);
    // Tick 뒤 현재 World의 ID·경계만 캡처하며 피킹은 Component에 위임한다.
    void CaptureWorld(UWorld& World);

    // 레이아웃과 Rect 상태를 기준으로 지정 View의 활성 여부를 반환한다.
    bool IsViewActive(int32 ViewIndex) const;
    // 현재 입력을 소비할 활성 View 인덱스를 반환한다.
    int32 GetActiveViewIndex() const { return ActiveViewIndex; }
    // 입력 영역 밖에서도 마지막 편집 대상을 유지하며 Single에서는 확대 View를 반환한다.
    int32 GetEditorViewIndex() const { return Views.Mode == ELayoutMode::Single ? SingleViewIndex : EditorViewIndex; }
    // 속성 창에서 선택한 View를 공통 편집 대상으로 지정한다.
    void SetEditorViewIndex(int32 Index) { if (Index >= 0 && Index < 4) EditorViewIndex = Index; }
    // View별 장면 래스터라이저 모드를 저장하고 조회한다.
    void SetViewWireframe(int32 Index, bool Value) { if (Index >= 0 && Index < 4) ViewWireframe[Index] = Value; }
    bool IsViewWireframe(int32 Index) const { return Index >= 0 && Index < 4 && ViewWireframe[Index]; }
    // 현재 마우스가 올라간 View 인덱스를 반환한다.
    int32 GetHoveredViewIndex() const { return InputState.HoveredViewIndex; }
    // 우클릭 입력을 Capture 중인 View 인덱스를 반환한다.
    int32 GetCapturedViewIndex() const { return InputState.CapturedViewIndex; }
    // 마지막 엔진 피킹 결과를 공통 Hit 형식으로 반환한다.
    const FPickHit& GetLastPick() const { return LastPick; }
    // 지정 View의 로컬 화면 Rect를 반환한다.
    const FRect& GetViewRect(int32 ViewIndex) const;
    // Native View·Projection을 row-vector 순서로 합성한 엔진 행렬을 반환한다.
    FMatrix GetEngineViewProjection(int32 ViewIndex) const;
    // 지정 View 카메라 위치를 엔진 FVector 그대로 반환한다.
    FVector GetEngineCameraLocation(int32 ViewIndex) const;
    // 지정 View의 카메라 Forward를 엔진 FVector로 계산해 반환한다.
    FVector GetEngineCameraForward(int32 ViewIndex) const;
    // Core Billboard 계산 결과를 엔진 월드 행렬로 변환한다.
    FMatrix BuildEngineBillboardMatrix(int32 ViewIndex, const FVector& WorldPosition, float Width, float Height) const;
    // 지정 View가 직교 투영인지 반환한다.
    bool IsOrthographic(int32 ViewIndex) const;
    // 활성 View의 로컬 마우스 좌표를 Core로 역투영해 엔진 Ray로 반환한다.
    bool TryGetActiveViewRay(FVector2 LocalMousePosition, FRay& OutRay) const;
    // 지정 View의 절두체를 통과한 오브젝트 수를 반환한다.
    std::size_t GetVisibleObjectCount(int32 ViewIndex) const;
    // 최근 World 스냅샷에 Billboard가 포함됐는지 반환한다.
    bool HasCapturedBillboard() const { return bCapturedBillboard; }
    // 최근 World 스냅샷에 Particle이 포함됐는지 반환한다.
    bool HasCapturedParticle() const { return bCapturedParticle; }

    // View별 가시 ID를 엔진 컴포넌트로 역매핑해 렌더 큐를 구성한다.
    void BuildRenderQueue(int32 ViewIndex, FRenderQueue& OutQueue);
    // 활성 View Ray를 World·Component 피킹으로 전달하고 마지막 결과를 보관한다.
    FPickHit PickActiveView(FVector2 LocalMousePosition, UWorld& World);
    // 마지막 Hit Component의 Owner를 찾아 Outliner 선택에 반영한다.
    void ApplyLastPickToOutliner(FOutlinerPanel& OutlinerPanel) const;

    // 외부에서 읽은 Split 비율을 Core 허용 범위로 제한해 저장한다.
    void SetSplitRatio(const FSplitRatio& Value);
    // 현재 가로·세로 Split 비율을 반환한다.
    const FSplitRatio& GetSplitRatio() const { return SplitRatio; }

private:
    // 직교 View의 논리 위치는 유지하고 렌더·컬링·피킹용 깊이 범위만 확장한다.
    FViewCamera GetRenderCamera(int32 ViewIndex) const;
    // 현재 호출에서 계산한 행렬·절두체를 보관한다.
    struct PreparedView
    {
        FFrustumPlanes Frustum{};
        FMatrix EngineViewProjection{};
    };
    // 호출할 때마다 현재 카메라로 VP와 절두체를 계산한다.
    const PreparedView& PrepareView(int32 ViewIndex) const;
    mutable PreparedView PreparedViews[4]{};
    static constexpr float MinimumSplitRatio = 0.1f;

    FViewSet Views{};
    EMultipleViewportsCameraPreset CameraPresets[4]{};
    FSplitRatio SplitRatio{};
    FViewInputState InputState{};
    FRect ViewRects[4]{};
    int32 SingleViewIndex = 0;
    int32 ActiveViewIndex = InvalidViewIndex;
    int32 EditorViewIndex = 0;
    bool ViewWireframe[4]{};
    FPickHit LastPick{};

    // 이번 프레임의 엔진 객체와 파티클 준비 상태를 보관한다. 포인터는 다음 캡처 전까지 유효해야 한다.
    struct PrimitiveSnapshot
    {
        UPrimitiveComponent* Primitive = nullptr;
        bool bCaptured = false;
        bool bParticlesPrepared = false;
        TArray<int32> AliveParticleIndices;
    };
    TMap<ObjectId, PrimitiveSnapshot> PrimitiveById;
    // Host가 컬링 입력 버퍼를 소유하고 용량을 재사용한다.
    TArray<FRenderableObject> RenderObjects;
    // 불투명 파티클은 최종 렌더러가 거리 정렬하지 않아 기존 Core 정렬을 유지한다.
    TArray<FParticleSortInput> SortInputs;
    TArray<ObjectId> SortedParticleIds;
    TArray<ObjectId> VisibleIds[4];
    TArray<UStaticMeshComponent*> PendingStaticMeshes;
    TArray<FLODSelectionInput> LODInputs;
    TArray<uint8> SelectedLODs;
    bool bCapturedBillboard = false;
    bool bCapturedParticle = false;
};
