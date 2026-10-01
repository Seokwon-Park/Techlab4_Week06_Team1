#include "EnginePCH.h"

#include "RenderCommand.h"

#include "Material.h"
#include "ObjectSystem/ObjectFactory.h"

#include "Asset/AssetManager.h"
#include "Serialization/TypeSerializer.h"

namespace
{
	// 저장된 경로로 텍스처를 찾고, 아직 로드 전이면 로드한다
	UTexture2D* FindOrLoadTexture(const FString& Path)
	{
		if (UTexture2D* Texture = UAssetManager::GetAssetByPath<UTexture2D>(Path))
		{
			return Texture;
		}
		return UAssetManager::Get().LoadTexture(Path);
	}
}

UMaterial* UMaterial::CreateInstance(const UMaterial* Source)
{
	if (Source == nullptr)
	{
		return nullptr;
	}

	UMaterial* Instance = FObjectFactory::ConstructObject<UMaterial>();

	Instance->ParamLayout = Source->ParamLayout;
	Instance->Shader = Source->Shader;
	Instance->Textures = Source->Textures;
	Instance->BlendState = Source->BlendState;
	Instance->DepthStencilState = Source->DepthStencilState;
	Instance->SamplerState = Source->SamplerState;
	Instance->BaseColor = Source->BaseColor;
	Instance->UVScrollSpeed = Source->UVScrollSpeed;

	// ParamBuffer는 TUniquePtr라 복사할 수 없다.
	// 원본이 갖고 있으면 같은 크기로 새로 만들어 준다. 내용은 매 프레임 갱신되므로 옮기지 않는다.
	if (Source->ParamBuffer)
	{
		Instance->ParamBuffer = RenderCommand::CreateConstantBuffer(Source->ParamBuffer->GetBufferSize());
	}

	Instance->BaseColor = Source->BaseColor;
	Instance->bIsInstance = true;
	Instance->Parent = Source;

	return Instance;
}

const UMaterial* UMaterial::GetBaseAsset() const
{
	for (const UMaterial* Current = this; Current; Current = Current->Parent)
	{
		if (!Current->GetPath().empty())
		{
			return Current;
		}
	}
	return nullptr;
}

// 머티리얼 하나를 JSON으로.
// 에셋이면 경로만, 경로 없는 인스턴스면 "기준 에셋 + 텍스처 + 색"으로 풀어서 쓴다
json UMaterial::SaveMaterial(const UMaterial* Material)
{
	json Out;

	if (!Material->GetPath().empty())
	{
		Out["Asset"] = Material->GetPath();
		return Out;
	}

	const UMaterial* Base = Material->GetBaseAsset();
	Out["Base"] = Base ? Base->GetPath() : FString("DefaultMaterial");
	Out["BaseColor"] = Material->BaseColor;
	Out["UVScrollSpeed"] = {
		Material->UVScrollSpeed.X,
		Material->UVScrollSpeed.Y
	};
	Out["SamplerState"] = Material->SamplerState == ESamplerState::LinearWrap ? "LinearWrap" : "LinearClamp";
	Out["BlendState"] = Material->BlendState == EBlendState::AlphaBlend ? "AlphaBlend" : "Opaque";

	json Textures = json::array();
	for (UTexture2D* Texture : Material->Textures)
	{
		if (Texture && !Texture->GetPath().empty())
			Textures.push_back(Texture->GetPath());
		else
			Textures.push_back(nullptr);
	}
	Out["Textures"] = Textures;

	return Out;
}

UMaterial* UMaterial::LoadMaterial(const json& In)
{
	if (In.contains("Asset"))
	{
		return UAssetManager::GetAssetByPath<UMaterial>(In["Asset"].get<FString>());
	}

	UMaterial* Base = nullptr;
	if (In.contains("Base"))
	{
		Base = UAssetManager::GetAssetByPath<UMaterial>(In["Base"].get<FString>());
	}
	if (!Base)
	{
		Base = UAssetManager::GetAssetByPath<UMaterial>("DefaultMaterial");
	}

	UMaterial* Instance = UMaterial::CreateInstance(Base);
	if (!Instance)
	{
		return nullptr;
	}

	if (In.contains("BaseColor"))
	{
		In["BaseColor"].get_to(Instance->BaseColor);
	}

	if (In.contains("Textures"))
	{
		Instance->Textures.Reset();
		for (const json& TexturePath : In["Textures"])
		{
			Instance->Textures.Add(TexturePath.is_null() ? nullptr : FindOrLoadTexture(TexturePath.get<FString>()));
		}
	}
	if (In.contains("UVScrollSpeed"))
	{
		const auto& UV = In["UVScrollSpeed"];
		Instance->UVScrollSpeed = FVector2(UV[0].get<float>(),UV[1].get<float>());
	}
	if (In.contains("SamplerState"))
	{
		const FString State = In["SamplerState"].get<FString>();
		Instance->SamplerState = State == "LinearWrap" ? ESamplerState::LinearWrap : ESamplerState::LinearClamp;
	}
	if (In.contains("BlendState"))
	{
		const FString State = In["BlendState"].get<FString>();
		Instance->BlendState = State == "AlphaBlend" ? EBlendState::AlphaBlend : EBlendState::Opaque;
	}

	return Instance;
}
