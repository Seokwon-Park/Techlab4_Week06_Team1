#pragma once

#include "Engine/Engine.h"
#include "Core/Window.h"
#include "Core/Types.h"

#include "Render/Renderer.h"
#include "Render/RenderDevice.h"
#include "Render/Swapchain.h"
#include "Editor/EditorUI/ImGuiRenderer.h"
#include "Editor/Rendering/GridRenderer.h"
#include "Editor/Gizmo/GizmoRenderer.h"
#include "Render/LineBatcher.h"
#include "Render/FogRenderer.h"
#include "Render/FXAARenderer.h"
#include "Render/DepthViewRenderer.h"


#include "Editor/EditorUI/EditorUI.h"
#include "Editor/OutputLog/OutputLogPanel.h"
#include "Editor/Details/DetailsPanel.h"
#include "Editor/EditorControls/EditorControlsPanel.h"
#include "Editor/Settings/SettingsPanel.h"
#include "Editor/Viewports/ViewportsPanel.h"
#include "Editor/LevelEditor/MultipleViewports/Adapter/MultipleViewportsAdapter.h"
#include "Editor/ContentDrawer/ContentDrawerPanel.h"

#include "Editor/Rendering/Outline.h"
#include "Editor/Rendering/OutLineRenderer.h"
#include "Editor/Outliner/OutlinerPanel.h"

#include "Render/SkyboxRenderer.h"

#include "PlayInEditorDataTypes.h"
#include "Core/Misc/Optional.h"

//Temp
#include "Text/Font.h"
#include "Text/TextRenderer.h"

class UEditorEngine : public UEngine
{
	DECLARE_CLASS(UEditorEngine, UEngine)

public:
	FEngineConfig GetConfig() const override;
	bool Init() override;
	void Tick(float DeltaTime) override;
	void PreExit() override;

	// Active View의 입력과 Picking 결과만 Gizmo 및 선택 상태에 반영한다.
	void UpdateGizmoAndPicking();
	// View 하나의 Scene·Grid·Gizmo·텍스트를 해당 ViewProjection으로 렌더한다.
	void RenderFrame(int32 ViewIndex, UWorld& ViewWorld, const FRenderingInfo& ViewRenderingInfo, const FMatrix& ViewProjection, const FVector& ViewCameraLocation, const FVector& ViewCameraForward, FRenderQueue& RenderQueue);
	// 네 View 결과와 ImGui를 메인 Swapchain 백버퍼에 합성한다. Present는 FEngineLoop가 한다.
	void PresentFrame();
	void DeleteActor(AActor* Actor);
	void DeleteComponent(UActorComponent* Component);

	//Play 버튼을 눌렀을때 Play Session 실행을 요청한다.
	inline void RequestPlaySession() { bPlaySessionRequested = true; }
	inline void RequestEndPlayMap() { bRequestEndPlayMapQueued = true; bIsPaused = false; }

	//실제 PIE 를 실행
	void StartPlayInEditorSession();
	UWorld* CreatePIEWorldByDuplication(FWorldContext& PIEContext, UWorld* InEditorWorld);

	void OnActiveWorldChanged();

	void EndPlayMap();

	bool IsPlaying() const { return PlayWorld != nullptr; }

	////FEditorDelegates::PrePIEEnded / EndPIE.Broadcast()
	//void TeardownPlaySession(const FWorldContext& PIEContext);            // 액터 EndPlay, 월드 정리(CleanupWorld)
	//void RestoreEditorWorld(UWorld* EditorWorld);            // GWorld를 에디터 월드로 복구
	//void DestroyWorldContext(UWorld* PlayWorld);

private:
	// 이번 프레임 DeltaTime을 패널에 전달하고 에디터 단축키를 처리한다.
	void BeginFrame(float DeltaTime);
	// 패널 요청과 입력을 Core Adapter에 전달해 레이아웃·카메라 상태를 갱신한다.
	void UpdateMultipleViewportState(float DeltaTime);
	// 월드를 정확히 한 번 Tick·Capture한 뒤 에디터 상호작용을 갱신한다.
	void TickWorldAndEditor(float DeltaTime);
	// 한 번 캡처한 월드 결과를 재사용해 현재 레이아웃의 각 View를 렌더한다.
	void RenderMultipleViewports();
	// PIE View가 PlayWorld 메인 카메라로 보이도록 Adapter Override를 갱신하고 게임 카메라 입력을 켜고 끈다.
	void UpdatePIEViewCamera();

	// 별도 PIE 창(New Editor Window)을 만들고 정리한다.
	bool CreatePIEWindow();
	void DestroyPIEWindow();
	// 창 크기에 맞춰 Swapchain과 오프스크린 타깃을 다시 만든다. 그릴 수 없으면 false다.
	bool UpdatePIEWindowTargets();
	// PIE 창의 오프스크린 결과를 백버퍼로 복사한다. Present는 EndFrame에서 한다.
	void CopyPIEWindowToBackbuffer();
	bool IsPIEInWindow() const { return PIEWindow != nullptr; }
	// View별 FXAA 임시 텍스처. 외부 View는 PIE 창 타깃을 쓴다.
	FTexture2D* GetFxaaTargetForView(int32 ViewIndex) const;
	// 화면 합성과 View 설정 보관으로 프레임을 마무리한다.
	void EndFrame();

	//불투명 물체에 대한 Pass
	void RenderOpaquePass(const int32 ViewIndex, const FRenderingInfo& ViewRenderingInfo, const FMatrix& ViewProjection, const FVector& ViewCameraLocation, const FVector& ViewCameraForward, FRenderQueue& RenderQueue);
	//안개에 대한 Pass
	void RenderFogPass(const int32 ViewIndex, UWorld& ViewWorld, const FRenderingInfo& ViewRenderingInfo, const FVector& ViewCameraLocation,const FMatrix& ViewProjection, FRenderQueue& RenderQueue);
	//Overlay Pass
	void RenderOverlayPass(const int32 ViewIndex, UWorld& ViewWorld, const FRenderingInfo& ViewRenderingInfo, const FMatrix& ViewProjection, const FVector& ViewCameraLocation, const FVector& ViewCameraForward, FRenderQueue& RenderQueue);
	//FXAA Pass
	void RenderFXAAPass(const int32 ViewIndex, const FRenderingInfo& ViewRenderingInfo);
	//Depth Pass
	void RenderDepthPass(const int32 ViewIndex, const FRenderingInfo& ViewRenderingInfo, const FMatrix& ViewProjection, const FVector& ViewCameraLocation, const FVector& ViewCameraForward);



	FWorldContext& GetEditorWorldContext();
	FWorldContext* GetPIEWorldContext(int32 WorldPIEInstance = 0);
	UWorld* GetActiveWorld() const { return PlayWorld ? PlayWorld : EditorWorld; }
	// PIE를 시작한 View만 PlayWorld를, 나머지 View는 EditorWorld를 그린다.
	UWorld* GetWorldForView(int32 ViewIndex) const { return PlayWorld && ViewIndex == PIEViewIndex ? PlayWorld : EditorWorld; }
	// PIE 중이고 일시정지가 아닌 View는 Gizmo·Picking 등 에디터 상호작용을 하지 않는다.
	bool IsPlayingView(int32 ViewIndex) const { return PlayWorld && ViewIndex == PIEViewIndex && !bIsPaused; }

	// FEngineLoop 소유. OnInit에서 받아 둔다.
	FWindow* MainWindow = nullptr;
	FSwapchain* MainWindowSC = nullptr;
	FRenderer* Renderer = nullptr;

	TUniquePtr<FEditorUI> EditorUI;

	TUniquePtr<FImGuiRenderer> ImGuiRenderer;
	TUniquePtr<FGridRenderer> GridRenderer;
	TUniquePtr<FGizmoRenderer> GizmoRenderer;
	TUniquePtr<FTextRenderer> TextRenderer;
	TUniquePtr<FFogRenderer> FogRenderer;
	TUniquePtr<FLineBatcher> LineBatcher;
	TUniquePtr<FGizmo> Gizmo;
	TUniquePtr<FOutline> Outline;
	TUniquePtr<FOutlineRenderer> OutlineRenderer;
	TUniquePtr<FSkyboxRenderer> SkyboxRenderer;
	TUniquePtr<FFXAARenderer> FXAARenderer;
	TUniquePtr<FDepthViewRenderer> DepthViewRenderer;

	UFont* SystemFont;

	FOutputLogPanel* OutputLogPanel = nullptr;

	FDetailsPanel* DetailsPanel = nullptr;
	FEditorControlsPanel* EditorControlsPanel = nullptr;
	FSettingsPanel* SettingsPanel = nullptr;
	FViewportsPanel* ViewportsPanel = nullptr;
	FMultipleViewportsAdapter MultipleViewportsAdapter;
	FRenderQueue RenderQueue;
	FOutlinerPanel* OutlinerPanel = nullptr;
	FContentDrawerPanel* ContentDrawerPanel = nullptr;

	UWorld* EditorWorld = nullptr;
	UWorld* PlayWorld = nullptr;
	// PIE 시작 시점의 편집 View. 별도 창이면 Adapter의 ExternalViewIndex다. PIE가 아니면 InvalidViewIndex다.
	int32 PIEViewIndex = InvalidViewIndex;

	// Play 버튼이 다음 세션을 띄울 위치
	EPlayModeType PlayMode = EPlayModeType::InViewport;

	// New Editor Window(PIE) 모드에서만 존재한다. Swapchain이 창 핸들을 쓰므로 창보다 먼저 해제한다.
	TUniquePtr<FWindow> PIEWindow;
	TUniquePtr<FSwapchain> PIESwapchain;
	TUniquePtr<FTexture2D> PIEColorTarget;
	TUniquePtr<FTexture2D> PIEDepthTarget;
	TUniquePtr<FTexture2D> PIEFxaaTarget;
	FRenderingInfo PIERenderingInfo{};
	uint32 PIETargetWidth = 0;
	uint32 PIETargetHeight = 0;
	bool bPIEWindowRendered = false;

	FVector UUIDLocation;

	FEditorCommand Save;

	FEditorCommand Play;
	FEditorCommand Pause;
	FEditorCommand Resume;
	FEditorCommand StepFrame;
	FEditorCommand Stop;

	bool bPlaySessionRequested = false;
	bool bRequestEndPlayMapQueued = false;
	bool bStepRequested = false;
	bool bIsPaused = false;

	void ResetSceneSelection();

	void CreateNewScene();
	void OpenScene();
	void SaveCurrentScene();
	void SaveSceneAs();
};
