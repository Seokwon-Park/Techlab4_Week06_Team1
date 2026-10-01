#pragma once

#include "RenderDevice.h"
#include "RenderingInfo.h"

class FWindow;

class FSwapchain
{
public:
public:
	FSwapchain(FRenderDevice* InRenderDevice, FWindow* InWindow);
	void CreateBackbuffer();
	~FSwapchain();

	// 스왑체인 크기 변경시
	void Resize(int32 Width, int32 Height);

	void SwapBuffers(uint32 SyncInterval = 1, uint32 Flags = 0);

	const FRenderingInfo& GetRenderingInfo() const { return RenderingInfo; }
	// Tearing을 허용하면 보더리스 창에서도 주사율 상한 없이 Present한다.
	bool IsTearingAllowed() const { return bAllowTearing; }
private:
	void ValidateRenderingInfo();

	FRenderDevice* RenderDevice;

	bool bAllowTearing = false;

	DXGI_SWAP_CHAIN_DESC Desc;
	ComPtr<IDXGISwapChain> Swapchain;

	TUniquePtr<FTexture2D> BackbufferTexture;

	FRenderingInfo RenderingInfo{};
};