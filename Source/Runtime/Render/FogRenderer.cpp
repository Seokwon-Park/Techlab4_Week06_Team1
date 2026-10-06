#include "EnginePCH.h"

#include "FogRenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"


bool FFogRenderer::Init(FRenderer* InRenderer)
{

	Shader = FRenderResourceManager::GetShaderProgram("Resources/Shader/ExponentialHeightFog.hlsl");
	if (!Shader)
	{
		HTR_LOG(Error, "[ExponentialHeightFog] shader not found");
		return false;
	}


	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FFogConstants));

	PipelineState.Shader = Shader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// 셰이더가 알파에 안개 양을 담아 보내므로, 장면 위에 섞어서 덮는다
	PipelineState.BlendState = EBlendState::AlphaBlend;


	return true;
}

void FFogRenderer::OnRender(FTexture2D* depthTexture, const FMatrix& ViewProjection, const FVector& CameraPosition, const FFogInfo& InFogInfo)
{

	//fogshader 바인드
	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindShaderResource(0, depthTexture, EShaderBindFlagBits::Pixel);

	FFogConstants Constants;
	Constants.InverseViewProjection = ViewProjection.Inverse();
	Constants.CameraPosition = CameraPosition;
	Constants.FogInfo = InFogInfo;
	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);
	RenderCommand::Draw(3);
}
