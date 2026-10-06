#include "EnginePCH.h"

#include "FXAARenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Texture2D.h"


bool FFXAARenderer::Init()
{

	fxaaShader = FRenderResourceManager::GetShaderProgram("Resources/Shader/FxaaShader.hlsl");
	if (!fxaaShader)
	{
		HTR_LOG(Error, "[FXAA] shader not found");
		return false;
	}

	ConstantBuffer = RenderCommand::CreateConstantBuffer(sizeof(FFXAAConstants));

    PipelineState.Shader = fxaaShader;
    PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// 결과로 덮어쓰는 후처리라 블렌딩,깊이 테스트가 필요 없음
	PipelineState.BlendState = EBlendState::Opaque;
	PipelineState.DepthStencilState = EDepthStencilState::Disabled;
    return true;
}

void FFXAARenderer::OnRender(FTexture2D* InputTexture)
{
	// 셰이더가 실제로 샘플링하는 텍스처 크기로 한 픽셀의 UV 크기를 구한다
	const uint32 Width = InputTexture->GetWidth();
	const uint32 Height = InputTexture->GetHeight();
	if (Width == 0 || Height == 0)
		return;

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindShaderResource(0, InputTexture, EShaderBindFlagBits::Pixel);
	RenderCommand::BindSamplerState(0, ESamplerState::LinearClamp, EShaderBindFlagBits::Pixel);

	FFXAAConstants Constants{};
	Constants.InverseScreenSize = { 1.0f / Width, 1.0f / Height };
	RenderCommand::UpdateBufferData(ConstantBuffer.get(), &Constants);
	RenderCommand::BindConstantBuffer(0, ConstantBuffer.get(), EShaderBindFlagBits::Pixel);

	RenderCommand::Draw(3);
}
