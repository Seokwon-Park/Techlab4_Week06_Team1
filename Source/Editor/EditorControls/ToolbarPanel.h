#pragma once

#include "Editor/EditorUI/EditorPanel.h"

class FToolbarPanel : public IEditorPanel
{
public:
	virtual ~FToolbarPanel() = default;
	virtual bool Init() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRender() override;

	bool IsOpen() const { return bIsOpen; }
	void SetOpen(bool bOpen) { bIsOpen = bOpen; }
	virtual const char* GetPanelName() const { return "ToolbarPanel"; }

	void SetPlayCallback(std::function<void()> InCallback) { OnPlay = InCallback; }
	void SetStopCallback(std::function<void()> InCallback) { OnStop = InCallback; }
	void SetIsPlayingQuery(std::function<bool()> InQuery) { IsPlaying = InQuery; }

private:
	std::function<void()> OnPlay;
	std::function<void()> OnStop;
	std::function<bool()> IsPlaying;

	bool bIsOpen = true;
};