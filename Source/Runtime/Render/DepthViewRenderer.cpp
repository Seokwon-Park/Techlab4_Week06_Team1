#include "EnginePCH.h"

#include "DepthViewRenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Render/Buffer.h"


bool FDepthViewRenderer::Init()
{
	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/DepthViewShader.hlsl");
	if (!Shader)
	{
		HTR_LOG(Error, "[ExponentialHeightFog] shader not found");
		return false;
	}


	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FDepthViewConstants));

	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	return true;
}

void FDepthViewRenderer::OnRender(FTexture2D* depthTexture, const FMatrix& ViewProjection, const FVector& CameraPosition, const FVector& ViewCameraForward)
{
	FDepthViewConstants Constants;
	Constants.InverseViewProjection = ViewProjection.Inverse();
	Constants.CameraPosition = CameraPosition;
	Constants.VisualizeRange = 50;
	Constants.CameraForward = ViewCameraForward;

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindShaderResource(0, depthTexture, EShaderBindFlagBits::Pixel);

	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);
	RenderCommand::Draw(3);


}
