#pragma once
#include "Math/Transform.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include <array>
#include <utility>

class FPrimitiveSceneProxy;
inline constexpr uint32 InvalidObjectSlot = ~0u;

struct FRenderPacket
{
    const FPrimitiveSceneProxy* Proxy = nullptr;
    const FMatrix* Model = nullptr;
    UStaticMesh* Mesh = nullptr;
    UMaterial* Material = nullptr;
    const void* MaterialParamData = nullptr;
    float CameraToParticleDistance = 0.0f;
    uint32 MaterialParamDataSize = 0;
    uint32 StartIndex = 0;
    uint32 IndexCount = 0; // Zero selects the full index buffer.
    uint32 Slot = InvalidObjectSlot;
    uint8 LODIndex = 0;
    bool bOccludedByGpu = false;
};

// Shared bindings, with one ordinary indexed draw per item.
struct FStaticDrawItem
{
    const FPrimitiveSceneProxy* Proxy;
    uint32 Slot;
    uint32 StartIndex;
    uint32 IndexCount;
    uint32 bOccludedByGpu;
};

struct FStaticDrawGroup
{
    UMaterial* Material = nullptr;
    UStaticMesh* Mesh = nullptr;
    uint8 LODIndex = 0;
    std::vector<FStaticDrawItem> Items;
};

// Owns the current frame's billboard/particle matrices together with their packets.
// Blocks keep matrix addresses stable when either array grows.
class FRenderQueue : public TArray<FRenderPacket>
{
    using FPackets = TArray<FRenderPacket>;
    static constexpr uint32 MatricesPerBlock = 256;
    using FMatrixBlock = std::array<FMatrix, MatricesPerBlock>;
    TArray<TUniquePtr<FMatrixBlock>> MatrixBlocks;
    uint32 MatrixCount = 0;
public:
    FRenderQueue() = default;
    FRenderQueue(const FRenderQueue&) = delete;
    FRenderQueue& operator=(const FRenderQueue&) = delete;
    FRenderQueue(FRenderQueue&& Other) noexcept
        : FPackets(std::move(Other)), MatrixBlocks(std::move(Other.MatrixBlocks)),
          MatrixCount(std::exchange(Other.MatrixCount, 0)) {}
    FRenderQueue& operator=(FRenderQueue&& Other) noexcept
    {
        if (this != &Other)
        {
            FPackets::operator=(std::move(Other));
            MatrixBlocks = std::move(Other.MatrixBlocks);
            MatrixCount = std::exchange(Other.MatrixCount, 0);
        }
        return *this;
    }
    const FMatrix* StoreWorldMatrix(const FMatrix& World)
    {
        const uint32 BlockIndex = MatrixCount / MatricesPerBlock;
        if (BlockIndex >= static_cast<uint32>(MatrixBlocks.Num()))
            MatrixBlocks.Add(MakeUnique<FMatrixBlock>());
        FMatrix& Stored = (*MatrixBlocks[BlockIndex])[MatrixCount % MatricesPerBlock];
        Stored = World;
        ++MatrixCount;
        return &Stored;
    }
    void Reset()
    {
        FPackets::Reset();
        MatrixCount = 0;
    }
};
