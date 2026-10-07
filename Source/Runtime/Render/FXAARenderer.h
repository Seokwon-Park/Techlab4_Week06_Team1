#pragma once
#include "Render/PipelineState.h"
#include "Render/Buffer.h"
class FTexture2D;

struct FFXAAConstants
{
	FVector2 InverseScreenSize;
	FVector2 Padding;
};

class FFXAARenderer
{
public:
	FFXAARenderer() = default;
	~FFXAARenderer() = default;

	bool Init();
	// 현재 바인딩된 렌더 타깃에 InputTexture를 FXAA 처리해 그린다. 렌더 패스는 호출하는 쪽이 연다.
	void OnRender(FTexture2D* InputTexture);

	FFXAARenderer(const FFXAARenderer&) = delete;
	FFXAARenderer& operator=(const FFXAARenderer&) = delete;


private:
	FShaderProgram* fxaaShader = nullptr;
	FPipelineState PipelineState;
	TUniquePtr<FConstantBuffer> ConstantBuffer;

};
