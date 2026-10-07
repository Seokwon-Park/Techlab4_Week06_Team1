#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "Texture2D.h"

	struct FDepthViewConstants
	{
		FMatrix InverseViewProjection;   
		FVector CameraPosition;         
		float   VisualizeRange;        
		FVector CameraForward;           
		float   Padding;               
	};

class FDepthViewRenderer
{
public:
	FDepthViewRenderer() = default;
	~FDepthViewRenderer() = default;

	FDepthViewRenderer(const FDepthViewRenderer&) = delete;
	FDepthViewRenderer operator=(const FDepthViewRenderer&) = delete;

	bool Init();
	void OnRender(FTexture2D* depthTexture, const FMatrix& ViewProjection, const FVector& CameraPosition, const FVector& ViewCameraForward);
private:
	FPipelineState PipelineState;
	FShaderProgram* Shader = nullptr;
	TUniquePtr<FConstantBuffer> ConstantBuffer;

};