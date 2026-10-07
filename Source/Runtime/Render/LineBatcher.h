#pragma once

#include "Buffer.h"
#include "StaticMeshData.h"
#include "PipelineState.h"

class FRenderer;
class UWorld;

class FLineBatcher
{
public:
	FLineBatcher() = default;
	~FLineBatcher();
	bool Init(FRenderer* InRenderer, UWorld* InWorld);
	// InWorld에 속한 컴포넌트의 바운딩 박스만 쌓는다. 에디터·PIE 월드가 함께 살아 있을 수 있다.
	void BuildVertexBuffer(const UWorld& InWorld);

	void BeginFrame();


	void AddLine(const FVector& A, const FVector& B, const FVector4& Color);
	void AddBox(FBox BoundingBox, FVector4 Color);

	// 로컬 AABB의 모서리를 각각 월드로 옮겨 그린다. 회전이 살아있는 OBB가 된다.
	void AddOrientedBox(const FBox& LocalBox, const FMatrix& WorldMatrix, const FVector4& Color);
	void AddPath(const TArray<FVector>& Points, const FVector4& Color);

	void Flush();
	void OnRender(const FMatrix& ViewProjection);

private:
	FRenderer* Renderer;

	UWorld* World;

	FShaderProgram* LineShader;
	FPipelineState PipelineState;
	FVertex* VertexBufferBase;  
	FVertex* VertexBufferPtr;  
	TUniquePtr<FConstantBuffer> CB;
	uint32 VertexCount;

	uint32 MaxVertices = 0;
	TUniquePtr<FVertexBuffer> VertexBuffer;
};