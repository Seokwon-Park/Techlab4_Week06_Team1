#pragma once
#include "Render/Buffer.h"
#include "Render/PipelineState.h"
#include "Core/EngineString.h"
#include "Render/Renderer.h"
#include "Render/FogInfo.h"

struct FFogConstants
{
	FMatrix InverseViewProjection;
	FVector CameraPosition;
	FFogInfo FogInfo;
	FVector Padding = { 0.0f, 0.0f, 0.0f };
};


class FFogRenderer
{
public:
	FFogRenderer() = default;
	~FFogRenderer() = default;


	FFogRenderer(const FFogRenderer&) = delete;
	FFogRenderer operator=(const FFogRenderer&) = delete;	

	bool Init(FRenderer* InRenderer);
	void OnRender(FTexture2D* depthTexture, const FMatrix& ViewProjection, const FVector& CameraPosition, const FFogInfo& InFogInfo);
private:

	
	FShaderProgram* Shader = nullptr;
	FPipelineState PipelineState;
	TUniquePtr<FConstantBuffer> ConstantBuffer;

};