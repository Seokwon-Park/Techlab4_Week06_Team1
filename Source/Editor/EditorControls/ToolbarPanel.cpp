#include "EnginePCH.h"
#include "ToolbarPanel.h"

bool FToolbarPanel::Init()
{
    return false;
}

void FToolbarPanel::Tick(float DeltaTime)
{
}

void FToolbarPanel::OnRender()
{
    ImGui::Begin(GetPanelName(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    
    const bool bPlaying = IsPlaying && IsPlaying();

	ImGui::BeginDisabled(bPlaying);
	if (ImGui::Button("Play")) { if (OnPlay) OnPlay(); }
	ImGui::EndDisabled();

	ImGui::SameLine();

	ImGui::BeginDisabled(!bPlaying);
	if (ImGui::Button("Stop")) { if (OnStop) OnStop(); }
	ImGui::EndDisabled();

    ImGui::End();
}
