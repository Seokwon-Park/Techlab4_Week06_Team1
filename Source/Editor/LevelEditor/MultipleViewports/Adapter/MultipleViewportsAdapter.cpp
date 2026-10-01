#include "EnginePCH.h"

#include "Editor/LevelEditor/MultipleViewports/Adapter/MultipleViewportsAdapter.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/BillboardComponent.h"
#include "Component/ParticleSubUVComponent.h"
#include "Core/Stats/LightweightStats.h"
#include "Core/Stats/EditorStats.h"
#include "Editor/Outliner/OutlinerPanel.h"
#include "Editor/Rendering/GridRenderer.h"
#include "Engine/PrimitiveSceneProxy.h"
#include "Engine/World.h"
#include "Input/InputSystem.h"
#include "UObject/UObjectIterator.h"
#include "Asset/LOD/StaticMeshLODSelector.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace
{
constexpr float Pi = 3.14159265358979323846f;
constexpr float MaximumCameraPitchDegrees = 89.0f;
constexpr float MaximumRotationDegreesPerFrame = 45.0f;
constexpr float MouseWheelDeltaPerStep = 120.0f;
constexpr float OrthographicZoomFactorPerStep = 0.9f;
constexpr float MinimumOrthographicWidth = 0.01f;
// 양방향 깊이 보정 뒤에도 절두체 평면 법선이 Core epsilon(1e-6)보다 충분히 크게 유지되게 한다.
// 정사각 View의 최악 조건에서 깊이 계수는 약 1 / (4 * sqrt(2) * Span)이다.
constexpr float MaximumOrthographicSpan = 80000.0f;

// 외부 설정과 화면 비율을 반영해 직교 투영 폭이 안전한 범위를 벗어나지 않게 한다.
void ConstrainOrthographicWidth(FCameraProjection& Projection, const FRect& Rect)
{
    const float AspectRatio = Rect.Width > 0.0f && Rect.Height > 0.0f
        ? Rect.Width / Rect.Height : 1.0f;
    // 폭과 높이 모두 상한 안에 들도록 세로로 긴 View에서는 폭 상한도 함께 줄인다.
    const float MaximumWidth = std::max(MinimumOrthographicWidth,
        MaximumOrthographicSpan * std::min(1.0f, AspectRatio));
    Projection.OrthoWidth = std::isnan(Projection.OrthoWidth)
        ? 10.0f
        : FMath::Clamp(Projection.OrthoWidth, MinimumOrthographicWidth, MaximumWidth);
}

// 네 View Rect의 중앙 경계에서 절반씩 줄여 Splitter가 차지할 실제 빈 공간을 만든다.
void ApplySplitterGutter(const FVector2 WindowSize, FRect ViewRects[4])
{
    const float SplitX = ViewRects[0].Width;
    const float SplitY = ViewRects[0].Height;
    const float HalfThickness = SplitterThickness * 0.5f;
    const float HalfGapX = std::min({HalfThickness, SplitX, WindowSize.X - SplitX});
    const float HalfGapY = std::min({HalfThickness, SplitY, WindowSize.Y - SplitY});
    const float RightX = SplitX + HalfGapX;
    const float BottomY = SplitY + HalfGapY;
    const float LeftWidth = SplitX - HalfGapX;
    const float TopHeight = SplitY - HalfGapY;
    const float RightWidth = WindowSize.X - RightX;
    const float BottomHeight = WindowSize.Y - BottomY;

    ViewRects[0] = {0.0f, 0.0f, LeftWidth, TopHeight};
    ViewRects[1] = {RightX, 0.0f, RightWidth, TopHeight};
    ViewRects[2] = {0.0f, BottomY, LeftWidth, BottomHeight};
    ViewRects[3] = {RightX, BottomY, RightWidth, BottomHeight};
}

// 양·음 방향 키 상태 차이로 -1~1 축 입력을 만든다.
float AxisValue(const EKeyCode Positive, const EKeyCode Negative)
{
    return (FInputSystem::IsKeyDown(Positive) ? 1.0f : 0.0f) - (FInputSystem::IsKeyDown(Negative) ? 1.0f : 0.0f);
}

// 길이가 0이면 영벡터를, 아니면 정규화 벡터를 반환한다.
FVector NormalizedOrZero(const FVector Value)
{
    const float LengthSquared = Value.X * Value.X + Value.Y * Value.Y + Value.Z * Value.Z;
    if (LengthSquared <= 0.0f) return {};
    const float InverseLength = 1.0f / std::sqrt(LengthSquared);
    return {Value.X * InverseLength, Value.Y * InverseLength, Value.Z * InverseLength};
}

// quaternion으로 회전된 카메라 Forward의 Z 성분을 계산한다.
float CameraForwardZ(const FQuat& Rotation)
{
    return 2.0f * (Rotation.X * Rotation.Z - Rotation.W * Rotation.Y);
}

// +X Forward를 quaternion으로 회전해 카메라 월드 Forward를 구한다.
FVector CameraForward(const FQuat& Rotation)
{
    return {
        1.0f - 2.0f * (Rotation.Y * Rotation.Y + Rotation.Z * Rotation.Z),
        2.0f * (Rotation.X * Rotation.Y + Rotation.W * Rotation.Z),
        2.0f * (Rotation.X * Rotation.Z - Rotation.W * Rotation.Y)};
}

// +Y Right를 quaternion으로 회전해 카메라 화면의 수평 월드축을 구한다.
FVector CameraRight(const FQuat& Rotation)
{
    return {
        2.0f * (Rotation.X * Rotation.Y - Rotation.W * Rotation.Z),
        1.0f - 2.0f * (Rotation.X * Rotation.X + Rotation.Z * Rotation.Z),
        2.0f * (Rotation.Y * Rotation.Z + Rotation.W * Rotation.X)};
}

// +Z Up을 quaternion으로 회전해 카메라 화면의 수직 월드축을 구한다.
FVector CameraUp(const FQuat& Rotation)
{
    return {
        2.0f * (Rotation.X * Rotation.Z + Rotation.W * Rotation.Y),
        2.0f * (Rotation.Y * Rotation.Z - Rotation.W * Rotation.X),
        1.0f - 2.0f * (Rotation.X * Rotation.X + Rotation.Y * Rotation.Y)};
}

// 카메라 Forward의 XY 방향을 atan2로 바꿔 Euler Yaw를 구한다.
float CameraYawDegrees(const FQuat& Rotation)
{
    const FVector Forward = NormalizedOrZero(CameraForward(Rotation));
    return std::atan2(Forward.Y, Forward.X) * 180.0f / Pi;
}

// 카메라 Forward의 Z 성분을 asin해 Euler Pitch를 구한다.
float CameraPitchDegrees(const FQuat& Rotation)
{
    return std::asin(FMath::Clamp(CameraForwardZ(Rotation), -1.0f, 1.0f)) * 180.0f / Pi;
}

// Z-up Yaw와 Right축 Pitch만 합성해 Roll 없는 quaternion을 만든다.
FQuat MakeCameraRotation(const float YawDegrees, const float PitchDegrees)
{
    const float HalfYaw = YawDegrees * Pi / 360.0f;
    const float HalfPitch = PitchDegrees * Pi / 360.0f;
    const float SinYaw = std::sin(HalfYaw);
    const float CosYaw = std::cos(HalfYaw);
    const float SinPitch = std::sin(HalfPitch);
    const float CosPitch = std::cos(HalfPitch);

    // 카메라 입력은 Euler Yaw/Pitch 두 축으로만 계산한다. Core 경계에 전달할 때만 quaternion으로 변환한다.
    return {
        SinPitch * SinYaw,
        -SinPitch * CosYaw,
        CosPitch * SinYaw,
        CosPitch * CosYaw};
}

} // 익명 네임스페이스

// 메인 카메라 투영값을 공유하고 네 View의 기본 프리셋 상태를 만든다.
void FMultipleViewportsAdapter::InitializeFromWorld(UWorld& World)
{
    UCameraComponent* MainCamera = World.GetMainCamera() ? World.GetMainCamera()->GetCameraComponent() : nullptr;
    assert(MainCamera != nullptr);

    const FCameraProjection Perspective{
        EProjectionMode::Perspective,
        MainCamera->GetFieldOfView(),
        MainCamera->GetOrthoWidth(),
        MainCamera->GetNearZ(),
        MainCamera->GetFarZ()};
    Views.Mode = ELayoutMode::QuadSplit;
    // 최신 trace의 구도를 사용해 화면 정면은 +X, 화면 오른쪽은 +Y가 되도록 시작한다.
    Views.Cameras[0] = {{{-11.665390f, 6.117728f, 9.921079f},
        {0.081251f, 0.256328f, -0.291035f, 0.918146f}}, Perspective};
    Views.Cameras[0].Transform.Rotation = MakeCameraRotation(
        CameraYawDegrees(Views.Cameras[0].Transform.Rotation),
        CameraPitchDegrees(Views.Cameras[0].Transform.Rotation));
    CameraPresets[0] = EMultipleViewportsCameraPreset::Perspective;

    FCameraProjection Orthographic = Perspective;
    Orthographic.Mode = EProjectionMode::Orthographic;
    Views.Cameras[1] = {{{0.0f, 0.0f, 10.0f}, {0.0f, 0.70710678f, 0.0f, 0.70710678f}}, Orthographic}; // 위쪽에서 -Z 방향을 본다.
    Views.Cameras[2] = {{{10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}}, Orthographic}; // +X 쪽에서 -X 방향을 본다.
    Views.Cameras[3] = {{{0.0f, -10.0f, 0.0f}, {0.0f, 0.0f, 0.70710678f, 0.70710678f}}, Orthographic}; // -Y 쪽에서 +Y 방향을 본다.
    CameraPresets[1] = EMultipleViewportsCameraPreset::Top;
    CameraPresets[2] = EMultipleViewportsCameraPreset::Front;
    CameraPresets[3] = EMultipleViewportsCameraPreset::Left;
    for (int32 ViewIndex = 0; ViewIndex < 4; ++ViewIndex)
        ConstrainOrthographicWidth(Views.Cameras[ViewIndex].Projection, ViewRects[ViewIndex]);
}

// 범위를 검사한 뒤 Single 레이아웃 대상 View를 저장한다.
void FMultipleViewportsAdapter::SetSingleViewIndex(const int32 ViewIndex)
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    SingleViewIndex = ViewIndex;
}

// 카메라를 복사하고 Perspective이면 Forward 기준 Yaw·Pitch로 Roll을 제거한다.
void FMultipleViewportsAdapter::SetViewCamera(const int32 ViewIndex, const FViewCamera& Camera)
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    Views.Cameras[ViewIndex] = Camera;
    ConstrainOrthographicWidth(Views.Cameras[ViewIndex].Projection, ViewRects[ViewIndex]);
    if (Views.Cameras[ViewIndex].Projection.Mode == EProjectionMode::Perspective)
        Views.Cameras[ViewIndex].Transform.Rotation = MakeCameraRotation(
            CameraYawDegrees(Views.Cameras[ViewIndex].Transform.Rotation),
            CameraPitchDegrees(Views.Cameras[ViewIndex].Transform.Rotation));
    CameraPresets[ViewIndex] = Camera.Projection.Mode == EProjectionMode::Perspective
        ? EMultipleViewportsCameraPreset::Perspective
        : EMultipleViewportsCameraPreset::OrthographicView;
}

// 속성 편집을 적용하고 방향·투영 모드가 그대로이면 축 정렬 프리셋을 보존한다.
void FMultipleViewportsAdapter::ApplyCameraProperties(const int32 ViewIndex,
    const FViewCamera& Camera, const bool bRotationChanged)
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    const EMultipleViewportsCameraPreset PreviousPreset = CameraPresets[ViewIndex];
    const bool bKeepPreset = !bRotationChanged
        && Views.Cameras[ViewIndex].Projection.Mode == Camera.Projection.Mode;
    SetViewCamera(ViewIndex, Camera);
    if (bKeepPreset)
        CameraPresets[ViewIndex] = PreviousPreset;
}

// 범위를 검사한 뒤 지정 View 카메라의 const 참조를 반환한다.
const FViewCamera& FMultipleViewportsAdapter::GetViewCamera(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return Views.Cameras[ViewIndex];
}

// 축 정렬 프리셋은 고정 Transform을, Current는 현재 Transform을 유지해 적용한다.
void FMultipleViewportsAdapter::ApplyCameraPreset(const int32 ViewIndex, const EMultipleViewportsCameraPreset Preset)
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    FViewCamera& Camera = Views.Cameras[ViewIndex];
    ConstrainOrthographicWidth(Camera.Projection, ViewRects[ViewIndex]);
    CameraPresets[ViewIndex] = Preset;
    Camera.Projection.Mode = Preset == EMultipleViewportsCameraPreset::Perspective
        ? EProjectionMode::Perspective
        : EProjectionMode::Orthographic;

    switch (Preset)
    {
    case EMultipleViewportsCameraPreset::Top:
        Camera.Transform = {{0.0f, 0.0f, 10.0f}, {0.0f, 0.70710678f, 0.0f, 0.70710678f}};
        break;
    case EMultipleViewportsCameraPreset::Bottom:
        Camera.Transform = {{0.0f, 0.0f, -10.0f}, {0.0f, -0.70710678f, 0.0f, 0.70710678f}};
        break;
    case EMultipleViewportsCameraPreset::Front:
        Camera.Transform = {{10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};
        break;
    case EMultipleViewportsCameraPreset::Back:
        Camera.Transform = {{-10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}};
        break;
    case EMultipleViewportsCameraPreset::Left:
        Camera.Transform = {{0.0f, -10.0f, 0.0f}, {0.0f, 0.0f, 0.70710678f, 0.70710678f}};
        break;
    case EMultipleViewportsCameraPreset::Right:
        Camera.Transform = {{0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, -0.70710678f, 0.70710678f}};
        break;
    case EMultipleViewportsCameraPreset::Perspective:
        Camera.Transform.Rotation = MakeCameraRotation(
            CameraYawDegrees(Camera.Transform.Rotation),
            CameraPitchDegrees(Camera.Transform.Rotation));
        break;
    case EMultipleViewportsCameraPreset::OrthographicView:
        // 투영 방식만 바꾸는 프리셋은 현재 Transform을 의도적으로 유지한다.
        break;
    }
}

// 범위를 검사한 뒤 지정 View의 프리셋을 반환한다.
EMultipleViewportsCameraPreset FMultipleViewportsAdapter::GetCameraPreset(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return CameraPresets[ViewIndex];
}

// 축 정렬 프리셋은 고정 평면을, Current는 Forward 지배축에 수직인 평면을 고른다.
EGridPlane FMultipleViewportsAdapter::GetGridPlane(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    switch (CameraPresets[ViewIndex])
    {
    case EMultipleViewportsCameraPreset::Top:
    case EMultipleViewportsCameraPreset::Bottom:
        return EGridPlane::XY;
    case EMultipleViewportsCameraPreset::Front:
    case EMultipleViewportsCameraPreset::Back:
        return EGridPlane::YZ;
    case EMultipleViewportsCameraPreset::Left:
    case EMultipleViewportsCameraPreset::Right:
        return EGridPlane::XZ;
    case EMultipleViewportsCameraPreset::Perspective:
        return EGridPlane::XY;
    case EMultipleViewportsCameraPreset::OrthographicView:
        break;
    }

    const FVector Forward = CameraForward(Views.Cameras[ViewIndex].Transform.Rotation);
    const float AbsX = std::fabs(Forward.X);
    const float AbsY = std::fabs(Forward.Y);
    const float AbsZ = std::fabs(Forward.Z);
    if (AbsZ >= AbsX && AbsZ >= AbsY) return EGridPlane::XY;
    if (AbsY >= AbsX) return EGridPlane::XZ;
    return EGridPlane::YZ;
}

// Core Rect 계산 결과를 Single 대상 슬롯에 재배치하고 Hover·활성 View를 갱신한다.
void FMultipleViewportsAdapter::UpdateLayout(const FVector2 WindowSize, const FVector2 LocalMousePosition)
{
    ComputeViewRects(SplitRatio, WindowSize, ViewRects);
    if (Views.Mode == ELayoutMode::Single)
    {
        for (int32 ViewIndex = 0; ViewIndex < 4; ++ViewIndex)
            ViewRects[ViewIndex] = ViewIndex == SingleViewIndex
                ? FRect{0.0f, 0.0f, WindowSize.X, WindowSize.Y}
                : FRect{};
    }
    else
    {
        ApplySplitterGutter(WindowSize, ViewRects);
    }
    // 창 크기와 Single/Quad 전환으로 Aspect가 바뀌어도 저장값과 실제 투영값을 함께 보정한다.
    for (int32 ViewIndex = 0; ViewIndex < 4; ++ViewIndex)
        if (IsViewRectValid(ViewRects[ViewIndex]))
            ConstrainOrthographicWidth(Views.Cameras[ViewIndex].Projection, ViewRects[ViewIndex]);
    // Hover 추적은 비활성화한다. 클릭 위치 판정 함수는 입력 전환에서 계속 사용한다.
    // InputState.HoveredViewIndex = DetermineHoveredView(LocalMousePosition, ViewRects);
    InputState.HoveredViewIndex = InvalidViewIndex;
    if (InputState.CapturedViewIndex != InvalidViewIndex && !IsViewActive(InputState.CapturedViewIndex))
        InputState = EndCapture(InputState);
    // Capture가 없으면 마지막 선택을 유지하고, 숨겨진 View만 레이아웃에 맞게 보정한다.
    if (InputState.CapturedViewIndex != InvalidViewIndex)
        ActiveViewIndex = InputState.CapturedViewIndex;
    if (!IsViewActive(ActiveViewIndex))
        ActiveViewIndex = Views.Mode == ELayoutMode::Single && IsViewActive(SingleViewIndex)
            ? SingleViewIndex : InvalidViewIndex;
}

// Quad 레이아웃에서만 Core Splitter Drag 계산 결과를 상태에 반영한다.
void FMultipleViewportsAdapter::ApplySplitterDrag(const EDragAxis Axis, const float DeltaPixels, const FVector2 WindowSize)
{
    // 동명 멤버가 전역 계산 함수를 가리므로 이 호출에만 전역 한정이 필요하다.
    SplitRatio = ::ApplySplitterDrag(SplitRatio, Axis, DeltaPixels, WindowSize, MinimumSplitRatio);
    ComputeViewRects(SplitRatio, WindowSize, ViewRects);
}

// 우클릭 Capture 상태를 갱신하고 Perspective는 회전, Ortho는 화면 평면 드래그·폭 줌을 적용한다.
void FMultipleViewportsAdapter::UpdateInput(
    const float DeltaTime,
    const FVector2 LocalMousePosition,
    const float MoveSpeed,
    const float MouseSensitivity)
{
    const int WheelDelta = FInputSystem::GetWheelDelta(); // 이 Hook은 프레임마다 정확히 한 번만 호출한다.
    if (FInputSystem::IsMouseReleased(EMouseButton::Right))
        InputState = EndCapture(InputState);
    const int32 PointerViewIndex = DetermineHoveredView(LocalMousePosition, ViewRects);
    // 우클릭 시작 위치를 Capture하고, Capture 밖에서는 좌클릭으로만 선택을 바꾼다.
    if (InputState.CapturedViewIndex == InvalidViewIndex && IsViewActive(PointerViewIndex))
    {
        if (FInputSystem::IsMousePressed(EMouseButton::Right))
            InputState = BeginCapture(InputState, PointerViewIndex);
        else if (FInputSystem::IsMousePressed(EMouseButton::Left))
            ActiveViewIndex = PointerViewIndex;
    }
    if (InputState.CapturedViewIndex != InvalidViewIndex)
        ActiveViewIndex = InputState.CapturedViewIndex;
    if (!IsViewActive(ActiveViewIndex)) return;

    constexpr float PerspectiveWheelSpeed = 0.1f;
    FCameraMoveInput Input{};
    const bool bCameraCaptured = FInputSystem::IsMouseDown(EMouseButton::Right) &&
        InputState.CapturedViewIndex == ActiveViewIndex;
    const float ForwardAxis = bCameraCaptured ? AxisValue(EKeyCode::W, EKeyCode::S) : 0.0f;
    const float RightAxis = bCameraCaptured ? AxisValue(EKeyCode::D, EKeyCode::A) : 0.0f;
    const float VerticalAxis = bCameraCaptured ? AxisValue(EKeyCode::E, EKeyCode::Q) : 0.0f;
    const bool bOrthographic = Views.Cameras[ActiveViewIndex].Projection.Mode == EProjectionMode::Orthographic;
    // 직교 카메라 이동은 화면 평면으로 제한한다.
    // A/D는 수평, W/S는 수직으로 이동한다. 선택한 축 정렬 View가 뒤집히거나
    // 깊이축을 따라 이동하지 않도록 Q/E와 마우스 회전은 의도적으로 무시한다.
    const FVector Axis = bOrthographic
        ? NormalizedOrZero({0.0f, RightAxis, ForwardAxis})
        : NormalizedOrZero({ForwardAxis, RightAxis, VerticalAxis});
    Input.MoveAxis = {Axis.X * MoveSpeed, Axis.Y * MoveSpeed, Axis.Z * MoveSpeed};

    bool bUpdateEulerRotation = false;
    float UpdatedYawDegrees = 0.0f;
    float UpdatedPitchDegrees = 0.0f;
    if (!bOrthographic && bCameraCaptured)
    {
        const FQuat& CurrentRotation = Views.Cameras[ActiveViewIndex].Transform.Rotation;
        const float CurrentYawDegrees = CameraYawDegrees(CurrentRotation);
        const float CurrentPitchDegrees = CameraPitchDegrees(CurrentRotation);
        const float YawDeltaDegrees = FMath::Clamp(
            FInputSystem::GetMouseDeltaX() * MouseSensitivity,
            -MaximumRotationDegreesPerFrame,
            MaximumRotationDegreesPerFrame);
        const float PitchDeltaDegrees = FMath::Clamp(
            -FInputSystem::GetMouseDeltaY() * MouseSensitivity,
            -MaximumRotationDegreesPerFrame,
            MaximumRotationDegreesPerFrame);
        UpdatedYawDegrees = CurrentYawDegrees + YawDeltaDegrees;
        UpdatedPitchDegrees = FMath::Clamp(
            CurrentPitchDegrees + PitchDeltaDegrees,
            -MaximumCameraPitchDegrees,
            MaximumCameraPitchDegrees);
        bUpdateEulerRotation = true;
    }
    if (bOrthographic)
    {
        // 패널 밖이나 다른 View 위의 휠 입력은 마지막 선택 카메라에 전달하지 않는다.
        const int RoutedWheelDelta = bCameraCaptured || PointerViewIndex == ActiveViewIndex ? WheelDelta : 0;
        const float WheelSteps = static_cast<float>(RoutedWheelDelta) / MouseWheelDeltaPerStep;
        Views.Cameras[ActiveViewIndex].Projection.OrthoWidth = std::max(
            MinimumOrthographicWidth,
            Views.Cameras[ActiveViewIndex].Projection.OrthoWidth *
                std::pow(OrthographicZoomFactorPerStep, WheelSteps));
        const FRect& Rect = ViewRects[ActiveViewIndex];
        // 큰 휠 입력과 overflow도 동일한 안전 범위의 최댓값으로 제한한다.
        ConstrainOrthographicWidth(Views.Cameras[ActiveViewIndex].Projection, Rect);
    }
    else
    {
        Input.ZoomDelta = bCameraCaptured || PointerViewIndex == ActiveViewIndex
            ? static_cast<float>(WheelDelta) * PerspectiveWheelSpeed : 0.0f;
    }
    FViewCamera UpdatedCamera = ApplyCameraMovement(Views.Cameras[ActiveViewIndex], Input, DeltaTime);
    if (bOrthographic && bCameraCaptured)
    {
        const FRect& Rect = ViewRects[ActiveViewIndex];
        const float WorldUnitsPerPixel = Views.Cameras[ActiveViewIndex].Projection.OrthoWidth / Rect.Width;
        const FVector Right = NormalizedOrZero(CameraRight(Views.Cameras[ActiveViewIndex].Transform.Rotation));
        const FVector Up = NormalizedOrZero(CameraUp(Views.Cameras[ActiveViewIndex].Transform.Rotation));
        // 화면을 잡아 끄는 방향으로 보이도록 카메라는 수평 드래그의 반대, 수직 드래그의 Up 방향으로 이동한다.
        const FVector PanOffset = Right * (-static_cast<float>(FInputSystem::GetMouseDeltaX()) * WorldUnitsPerPixel)
            + Up * (static_cast<float>(FInputSystem::GetMouseDeltaY()) * WorldUnitsPerPixel);
        UpdatedCamera.Transform.Location += PanOffset;
    }
    if (bUpdateEulerRotation)
        UpdatedCamera.Transform.Rotation = MakeCameraRotation(UpdatedYawDegrees, UpdatedPitchDegrees);
    Views.Cameras[ActiveViewIndex] = UpdatedCamera;
}

// 현재 World의 가시 컴포넌트에서 경계만 캡처한다. Mesh·삼각형은 복사하지 않는다.
void FMultipleViewportsAdapter::CaptureWorld(UWorld& World)
{
    RenderObjects.Reset();
    for (auto& Entry : PrimitiveById) Entry.second.bCaptured = false;
    bCapturedBillboard = false;
    bCapturedParticle = false;

    const FScene& Scene = World.GetScene();
    const int32 Count = Scene.Proxies.Num();
    for (int32 i = 0; i < Count;++i)
    {
        if (!Scene.PrimitiveFlags[i]) continue;
        UPrimitiveComponent* Primitive = Scene.Proxies[i]->GetComponent();
        if (!Primitive || !Primitive->IsVisible() || !Primitive->GetOwner() ||
            Primitive->GetOwner()->GetWorld() != &World) continue;
        const ObjectId Id = Primitive->GetUUID();
        if (Id == InvalidObjectId) continue;

        FRenderableObject RenderObject{};
        RenderObject.Id = Id;
        const FPrimitiveSceneProxy* Proxy = Primitive->GetSceneProxy();
        RenderObject.WorldBounds = Proxy && Proxy->GetMesh()
            ? Proxy->GetBounds() : MakeWorldBounds(Primitive->CalcBounds());
        RenderObjects.Add(RenderObject);
        PrimitiveSnapshot& Snapshot = PrimitiveById[Id];
        Snapshot.Primitive = Primitive;
        Snapshot.bCaptured = true;
        Snapshot.bParticlesPrepared = false;
        if (Cast<UParticleSubUVComponent>(Primitive)) bCapturedParticle = true;
        else if (Cast<UBillboardComponent>(Primitive)) bCapturedBillboard = true;
    }

    for (auto Iterator = PrimitiveById.begin(); Iterator != PrimitiveById.end();)
    {
        const auto Current = Iterator++;
        if (!Current->second.bCaptured) PrimitiveById.Remove(Current->first);
    }
}

// 유효 Rect와 Single 대상 인덱스 또는 Quad 모드로 View 활성 여부를 판정한다.
bool FMultipleViewportsAdapter::IsViewActive(const int32 ViewIndex) const
{
    if (ViewIndex < 0 || ViewIndex >= 4 || !IsViewRectValid(ViewRects[ViewIndex]))
        return false;
    return Views.Mode == ELayoutMode::QuadSplit || ViewIndex == SingleViewIndex;
}

// 범위를 검사한 뒤 계산된 View Rect의 const 참조를 반환한다.
const FRect& FMultipleViewportsAdapter::GetViewRect(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return ViewRects[ViewIndex];
}

// 직교 화면의 XY는 유지하면서 렌더·컬링·피킹용 카메라만 후퇴시켜 양방향 깊이를 확보한다.
FViewCamera FMultipleViewportsAdapter::GetRenderCamera(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    FViewCamera Camera = Views.Cameras[ViewIndex];
    if (Camera.Projection.Mode != EProjectionMode::Orthographic)
        return Camera;

    const FRect& Rect = ViewRects[ViewIndex];
    const float AspectRatio = Rect.Width > 0.0f && Rect.Height > 0.0f
        ? Rect.Width / Rect.Height : 1.0f;
    const float HalfWidth = Camera.Projection.OrthoWidth * 0.5f;
    const float HalfHeight = HalfWidth / AspectRatio;
    const FVector Position = Camera.Transform.Location;
    const float Distance = std::sqrt(
        Position.X * Position.X + Position.Y * Position.Y + Position.Z * Position.Z);

    // 현재 카메라 평면의 앞뒤를 모두 포함하고 기존 FarClip보다 깊이 범위를 줄이지 않는다.
    const float Radius = std::max(Camera.Projection.FarClip,
        2.0f * Distance + 4.0f * std::sqrt(HalfWidth * HalfWidth + HalfHeight * HalfHeight));
    const FVector Forward = NormalizedOrZero(CameraForward(Camera.Transform.Rotation));
    const float Retreat = Radius + Camera.Projection.NearClip;
    Camera.Transform.Location = Position - Forward * Retreat;
    Camera.Projection.FarClip = Camera.Projection.NearClip + 2.0f * Radius;
    return Camera;
}

// 현재 카메라로 VP와 절두체를 매번 계산한다.
const FMultipleViewportsAdapter::PreparedView& FMultipleViewportsAdapter::PrepareView(const int32 ViewIndex) const
{
    assert(IsViewActive(ViewIndex));
    const FViewCamera Camera = GetRenderCamera(ViewIndex);
    const auto& Rect = ViewRects[ViewIndex];
    PreparedView& Prepared = PreparedViews[ViewIndex];
    Prepared.EngineViewProjection = BuildViewMatrix(Camera.Transform) *
        BuildProjectionMatrix(Camera.Projection, Rect.Width / Rect.Height);
    Prepared.Frustum = ExtractFrustumPlanes(Prepared.EngineViewProjection);
    return Prepared;
}

// Native와 엔진이 함께 쓰는 현재 row-vector VP를 반환한다.
FMatrix FMultipleViewportsAdapter::GetEngineViewProjection(const int32 ViewIndex) const
{
    assert(IsViewActive(ViewIndex));
    return PrepareView(ViewIndex).EngineViewProjection;
}

// Native 카메라 위치를 엔진 FVector 그대로 반환한다.
FVector FMultipleViewportsAdapter::GetEngineCameraLocation(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return Views.Cameras[ViewIndex].Transform.Location;
}

// Native 회전에서 정규화한 Forward를 엔진 FVector로 반환한다.
FVector FMultipleViewportsAdapter::GetEngineCameraForward(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return NormalizedOrZero(CameraForward(Views.Cameras[ViewIndex].Transform.Rotation));
}

// 위치·크기·View 카메라로 엔진 규약의 Billboard 행렬을 계산한다.
FMatrix FMultipleViewportsAdapter::BuildEngineBillboardMatrix(const int32 ViewIndex, const FVector& WorldPosition, const float Width, const float Height) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    const FViewCamera& ViewCamera = Views.Cameras[ViewIndex];
    if (ViewCamera.Projection.Mode == EProjectionMode::Orthographic)
    {
        // 직교 투영의 모든 시선은 평행하므로 카메라 위치가 아니라 고정된 화면 기저를 사용한다.
        // 위치 기반 LookAt을 사용하면 평면 이동 시 오브젝트-카메라 벡터가 달라져 Billboard가 회전한다.
        const FVector Facing = NormalizedOrZero(CameraForward(ViewCamera.Transform.Rotation)) * -1.0f;
        const FVector Right = NormalizedOrZero(CameraRight(ViewCamera.Transform.Rotation));
        const FVector Up = NormalizedOrZero(CameraUp(ViewCamera.Transform.Rotation));

        FMatrix EngineMatrix;
        EngineMatrix.SetIdentity();
        EngineMatrix.M[0][0] = Facing.X; EngineMatrix.M[0][1] = Facing.Y; EngineMatrix.M[0][2] = Facing.Z;
        EngineMatrix.M[1][0] = Right.X * Width; EngineMatrix.M[1][1] = Right.Y * Width; EngineMatrix.M[1][2] = Right.Z * Width;
        EngineMatrix.M[2][0] = Up.X * Height; EngineMatrix.M[2][1] = Up.Y * Height; EngineMatrix.M[2][2] = Up.Z * Height;
        EngineMatrix.M[3][0] = WorldPosition.X; EngineMatrix.M[3][1] = WorldPosition.Y; EngineMatrix.M[3][2] = WorldPosition.Z;
        return EngineMatrix;
    }

    const FBillboardTransform Result = ComputeBillboardTransform(
        {WorldPosition, {Width, Height}},
        ViewCamera.Transform);
    FMatrix EngineMatrix = Result.WorldMatrix;

    // 팀 엔진의 ParticleQuad는 Core의 Billboard 오른쪽 축과 반대 와인딩을 사용한다.
    // 오른쪽 축만 뒤집어 기존 렌더 경로와 같은 앞면이 카메라를 향하게 한다.
    EngineMatrix.M[1][0] = -EngineMatrix.M[1][0];
    EngineMatrix.M[1][1] = -EngineMatrix.M[1][1];
    EngineMatrix.M[1][2] = -EngineMatrix.M[1][2];
    return EngineMatrix;
}

// 지정 View 카메라의 투영 모드가 Orthographic인지 검사한다.
bool FMultipleViewportsAdapter::IsOrthographic(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return Views.Cameras[ViewIndex].Projection.Mode == EProjectionMode::Orthographic;
}

// 활성 View Rect 기준 로컬 좌표를 Core로 역투영하고 엔진 Ray로 변환한다.
bool FMultipleViewportsAdapter::TryGetActiveViewRay(const FVector2 LocalMousePosition, FRay& OutRay) const
{
    if (ActiveViewIndex == InvalidViewIndex || !IsViewActive(ActiveViewIndex))
        return false;
    // Splitter gutter나 다른 View 위의 좌표를 이전 Active View의 Ray로 잘못 해석하지 않는다.
    if (DetermineHoveredView(LocalMousePosition, ViewRects) != ActiveViewIndex)
        return false;
    const FRect& Rect = ViewRects[ActiveViewIndex];
    const FVector2 ViewLocal{LocalMousePosition.X - Rect.X, LocalMousePosition.Y - Rect.Y};
    const FRay Ray = Deproject(GetRenderCamera(ActiveViewIndex), ViewLocal, {Rect.Width, Rect.Height});
    OutRay = Ray;
    return true;
}

// 범위를 검사한 뒤 지정 View의 재사용 가시 ID 버퍼 크기를 반환한다.
std::size_t FMultipleViewportsAdapter::GetVisibleObjectCount(const int32 ViewIndex) const
{
    assert(ViewIndex >= 0 && ViewIndex < 4);
    return VisibleIds[ViewIndex].Num();
}

// 가시 ID를 컴포넌트로 역매핑해 큐를 구성한다. 생존 목록·상수는 프레임 공통, Billboard·정렬 거리는 View별이다.
void FMultipleViewportsAdapter::BuildRenderQueue(const int32 ViewIndex, FRenderQueue& OutQueue)
{
    OutQueue.Reset();
    PendingStaticMeshes.Reset();
    LODInputs.Reset();
    if (!IsViewActive(ViewIndex)) return;
    {
        CullForView(RenderObjects, PrepareView(ViewIndex).Frustum, VisibleIds[ViewIndex]);
    }
    const FRect& Rect = GetViewRect(ViewIndex);
    const FViewCamera& ViewCamera = Views.Cameras[ViewIndex];
    const FMatrix Projection = BuildProjectionMatrix(
        ViewCamera.Projection, Rect.Width / Rect.Height);
    const float ScaleX = Projection.M[1][0];
    const float ScaleY = Projection.M[2][1];

    FLODViewContext LODContext{
        static_cast<uint32>(Rect.Width),
        static_cast<uint32>(Rect.Height)
    };
    LODContext.CameraPosition = GetEngineCameraLocation(ViewIndex);
    LODContext.ViewProjection = GetEngineViewProjection(ViewIndex);
    LODContext.CameraForward = GetEngineCameraForward(ViewIndex);
    LODContext.ProjectionScaleSquared = std::max(ScaleX * ScaleX, ScaleY * ScaleY);
    LODContext.NearZ = ViewCamera.Projection.NearClip;
    LODContext.bOrthographic = ViewCamera.Projection.Mode == EProjectionMode::Orthographic;
    LODContext.Prepare();

    for (const ObjectId Id : VisibleIds[ViewIndex])
    {
        const auto Found = PrimitiveById.Find(Id);
        if (!Found || !Found->Primitive)
            continue;

        UPrimitiveComponent* Primitive = Found->Primitive;
        if (UParticleSubUVComponent* ParticleComponent = Cast<UParticleSubUVComponent>(Primitive))
        {
            const TArray<FParticle>& Particles = ParticleComponent->GetParticlesForView();
            PrimitiveSnapshot& Snapshot = *Found;
            if (!Snapshot.bParticlesPrepared)
            {
                Snapshot.AliveParticleIndices.Reset();
                Snapshot.AliveParticleIndices.Reserve(Particles.Num());
                for (int32 Index = 0; Index < Particles.Num(); ++Index)
                    if (Particles[Index].bAlive) Snapshot.AliveParticleIndices.Add(Index);
                ParticleComponent->BeginViewSubmission();
                Snapshot.bParticlesPrepared = true;
            }
            // 반투명은 모든 emitter를 합친 최종 Renderer만 정렬한다. 불투명은 Core 정렬을 유지하며 동률은 인덱스 순서다.
            const FVector CameraLocation = GetEngineCameraLocation(ViewIndex);
            const bool bOpaque = ParticleComponent->UsesOpaqueMaterial();
            if (bOpaque)
            {
                SortInputs.Reset();
                for (const int32 Index : Snapshot.AliveParticleIndices)
                    SortInputs.Add({static_cast<ObjectId>(Index + 1), Particles[Index].Location});
                SortParticlesByCameraDistance(SortInputs, Views.Cameras[ViewIndex].Transform.Location, SortedParticleIds);
            }
            for (int32 Order = 0; Order < Snapshot.AliveParticleIndices.Num(); ++Order)
            {
                const int32 ParticleIndex = bOpaque ? static_cast<int32>(SortedParticleIds[Order] - 1) : Snapshot.AliveParticleIndices[Order];
                const FParticle& Particle = Particles[ParticleIndex];
                const FMatrix ParticleWorld = BuildEngineBillboardMatrix(ViewIndex, Particle.Location, Particle.Scale, Particle.Scale);
                const FVector Delta = Particle.Location - CameraLocation;
                ParticleComponent->SubmitParticleToRenderQueue(OutQueue, ParticleIndex, ParticleWorld, Delta.Dot(Delta));
            }
        }
        else if (UBillboardComponent* Billboard = Cast<UBillboardComponent>(Primitive))
        {
            const FVector Scale = Billboard->GetWorldScale3D();
            Billboard->SubmitToRenderQueue(
                OutQueue,
                BuildEngineBillboardMatrix(ViewIndex, Billboard->GetWorldLocation(), Scale.Y, Scale.Z));
        }
        else if (auto* StaticComponent = Cast<UStaticMeshComponent>(Primitive))
        {
            UStaticMesh* Mesh = StaticComponent->GetStaticMesh();
            if (!Mesh)
                continue;

            const FPrimitiveSceneProxy* Proxy = StaticComponent->GetSceneProxy();
            if (Proxy)
            {
                PendingStaticMeshes.Add(StaticComponent);
                LODInputs.Add({Proxy->GetLODSphere(), Proxy->GetRenderState()});
            }
        }
        else
        {
            Primitive->SubmitToRenderQueue(OutQueue);
        }
    }
    SelectLODs(LODInputs, LODContext, SelectedLODs);
    for (uint32 I = 0; I < static_cast<uint32>(PendingStaticMeshes.Num()); ++I)
        PendingStaticMeshes[I]->SubmitToRenderQueue(OutQueue, SelectedLODs[I]);
}

// 클릭한 View의 Ray를 World에 전달하고 Component의 최근접 교차 결과를 보관한다.
FPickHit FMultipleViewportsAdapter::PickActiveView(const FVector2 LocalMousePosition, UWorld& World)
{
    SCOPE_CYCLE_COUNTER_ALWAYS(EditorStats::STAT_PickingTime);

	LastPick = {};
    FRay Ray{};
    if (!TryGetActiveViewRay(LocalMousePosition, Ray)) return LastPick;

    // 렌더와 같은 함수로 각 Billboard의 위치·크기에 맞는 View 행렬을 만든다.
    const auto ResolveBillboardTransform = [](const UBillboardComponent& Billboard, const void* Context) -> FMatrix
    {
        const auto& Adapter = *static_cast<const FMultipleViewportsAdapter*>(Context);
        const FVector Scale = Billboard.GetWorldScale3D();
        return Adapter.BuildEngineBillboardMatrix(Adapter.GetActiveViewIndex(),
            Billboard.GetWorldLocation(), Scale.Y, Scale.Z);
    };
    FHitResult Hit;
    if (World.LineTraceSingle(Ray, Hit, ResolveBillboardTransform, this))
    {
        LastPick.bHit = true;
        LastPick.Id = Hit.HitComponent->GetUUID();
        LastPick.Distance = Hit.Distance;
        LastPick.HitPoint = Hit.ImpactPoint;
    }
    return LastPick;
}

// 렌더 캡처 목록에 없는 컴포넌트도 Hit ID로 찾아 Owner를 선택한다. 삭제된 ID는 선택을 해제한다.
void FMultipleViewportsAdapter::ApplyLastPickToOutliner(FOutlinerPanel& OutlinerPanel) const
{
    if (LastPick.bHit)
    {
        for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
        {
            if (It->GetUUID() == LastPick.Id)
            {
                OutlinerPanel.SelectActor(It->GetOwner());
                return;
            }
        }
    }
    OutlinerPanel.SelectActor(nullptr);
}

// Config에서 받은 비율을 Core 최소 범위로 clamp해 Adapter 상태에 저장한다.
void FMultipleViewportsAdapter::SetSplitRatio(const FSplitRatio& Value)
{
    SplitRatio = ClampSplitRatio(Value, MinimumSplitRatio);
}
