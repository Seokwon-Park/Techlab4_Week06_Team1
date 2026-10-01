#include "EnginePCH.h"
#include "Texture2D.h"

#include "RenderCommand.h"


FTexture2D::FTexture2D(ID3D11Device* Device, const D3D11_TEXTURE2D_DESC& InDesc, const FImageData& Image)
	: FTexture2D(Device, InDesc, Image.IsValid() ? Image.Pixels.GetData() : nullptr, Image.GetRowPitch())
{
}

FTexture2D::FTexture2D(ID3D11Device* Device, const D3D11_TEXTURE2D_DESC& InDesc, const void* InitialData, uint32 RowPitch)
{
	Width = InDesc.Width;
	Height = InDesc.Height;
	Depth = 1;
	Format = InDesc.Format;
	Dimension = ETextureDimension::Texture2D;

	if (InitialData && RowPitch == 0)
	{
		const uint32 BytesPerPixel = FormatToBytes(InDesc.Format);
		if (BytesPerPixel == 0) { /* 기존 에러 처리 그대로 */ return; }
		RowPitch = InDesc.Width * BytesPerPixel;
	}

	const bool bGenerateMips = (InDesc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS) != 0;

	D3D11_SUBRESOURCE_DATA SubData{ InitialData, RowPitch, 0 };
	const D3D11_SUBRESOURCE_DATA* InitPtr = (InitialData && !bGenerateMips) ? &SubData : nullptr;

	HRESULT hr = Device->CreateTexture2D(&InDesc, InitPtr, (ID3D11Texture2D**)Texture.GetAddressOf());
	if (FAILED(hr)) { /* 기존 로그 그대로 */ return; }

	CreateViews(Device, InDesc);

	if (bGenerateMips && InitialData && SRV)
	{
		ComPtr<ID3D11DeviceContext> Context;
		Device->GetImmediateContext(Context.GetAddressOf());
		Context->UpdateSubresource(Texture.Get(), 0, nullptr, InitialData, RowPitch, 0); // 0번 레벨
		Context->GenerateMips(SRV.Get());                                              // 나머지 레벨
	}
}
FTexture2D::FTexture2D(ID3D11Device* Device, ComPtr<ID3D11Resource> SwapchainTexture, const D3D11_TEXTURE2D_DESC& InDesc)
{
	Texture = std::move(SwapchainTexture);

	Width = InDesc.Width;
	Height = InDesc.Height;
	Depth = 1;
	Format = InDesc.Format;
	Dimension = ETextureDimension::Texture2D;

	CreateViews(Device, InDesc);
}

void FTexture2D::CreateViews(ID3D11Device* Device, const D3D11_TEXTURE2D_DESC& InDesc)
{
	HRESULT hr;
	if (InDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		hr = Device->CreateShaderResourceView(Texture.Get(), nullptr, SRV.GetAddressOf());
		if (FAILED(hr))
			HTR_LOG(Error, "[Texture2D] CreateShaderResourceView failed (hr=0x{:08X})", (uint32)hr);
	}

	if ((InDesc.BindFlags & D3D11_BIND_RENDER_TARGET) &&
		!(InDesc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS))
	{
		hr = Device->CreateRenderTargetView(Texture.Get(), nullptr, RTV.GetAddressOf());
		if (FAILED(hr))
			HTR_LOG(Error, "[Texture2D] CreateRenderTargetView failed (hr=0x{:08X})", (uint32)hr);
	}

	if (InDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		hr = Device->CreateDepthStencilView(Texture.Get(), nullptr, DSV.GetAddressOf());
		if (FAILED(hr))
			HTR_LOG(Error, "[Texture2D] CreateDepthStencilView failed (hr=0x{:08X})", (uint32)hr);
	}
}

UTexture2D::UTexture2D()
{

}
UTexture2D::~UTexture2D()
{
}

