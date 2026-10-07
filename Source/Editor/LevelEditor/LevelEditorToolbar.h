#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "Editor/EditorUI/EditorPanel.h"
#include "Editor/EditorUI/EditorCommand.h"

class FLevelEditorToolbar
{
public:
	~FLevelEditorToolbar() = default;

	void SetCommands(const TArray<FEditorCommand>& InCommands) { Commands = InCommands; }
	void Draw();

	static constexpr float Height = 40.0f;

private:
	void DrawFileGroup();
	void DrawPlayGroup();

	bool bIsOpen = true;
	TArray<FEditorCommand> Commands;


};