#include "EnginePCH.h"
#include "JsonArchiveOutputFormatter.h"

#include "Asset/RenderAsset.h"

FJsonArchiveOutputFormatter::FJsonArchiveOutputFormatter(FArchive& InInner, FJsonValue InRoot)
	: Inner(InInner), Root(InRoot) 
{
	ValueStack.Add(&InRoot);
};

void FJsonArchiveOutputFormatter::EnterRecord()
{
	Top() = json::object();
}

void FJsonArchiveOutputFormatter::LeaveRecord()
{
}

void FJsonArchiveOutputFormatter::EnterField(FArchiveFieldName Name)
{
	json& Object = Top();
	assert(Object.is_object() && "EnterRecord 없이 EnterField");
	ValueStack.Add(&Object[Name.Name]);      
}

void FJsonArchiveOutputFormatter::LeaveField()
{
	ValueStack.Pop();
}


bool FJsonArchiveOutputFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving)
{
	return false;
}

void FJsonArchiveOutputFormatter::EnterArray(int32& NumElements)
{
}

void FJsonArchiveOutputFormatter::LeaveArray()
{
}

void FJsonArchiveOutputFormatter::EnterArrayElement()
{
}

void FJsonArchiveOutputFormatter::LeaveArrayElement()
{
}

void FJsonArchiveOutputFormatter::EnterStream()
{
}

void FJsonArchiveOutputFormatter::LeaveStream()
{
}

void FJsonArchiveOutputFormatter::EnterStreamElement()
{
}

void FJsonArchiveOutputFormatter::LeaveStreamElement()
{
}

void FJsonArchiveOutputFormatter::EnterMap(int32& NumElements)
{
}

void FJsonArchiveOutputFormatter::LeaveMap()
{
}

void FJsonArchiveOutputFormatter::EnterMapElement(FString& Name)
{
}

void FJsonArchiveOutputFormatter::LeaveMapElement()
{
}

void FJsonArchiveOutputFormatter::EnterAttributedValue()
{
}

void FJsonArchiveOutputFormatter::EnterAttributedValueValue()
{
}

void FJsonArchiveOutputFormatter::LeaveAttribute()
{
}

void FJsonArchiveOutputFormatter::LeaveAttributedValue()
{
}

bool FJsonArchiveOutputFormatter::TryEnterAttributedValueValue()
{
	return false;
}

// ── 값 ──
void FJsonArchiveOutputFormatter::Serialize(uint8& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(uint16& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(uint32& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(uint64& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(int8& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(int16& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(int32& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(int64& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(bool& Value) { Top() = Value; }
void FJsonArchiveOutputFormatter::Serialize(FString& Value) { Top() = Value; }

void FJsonArchiveOutputFormatter::Serialize(float& Value)
{
	// nlohmann은 NaN/무한대를 null로 출력한다 → 불러올 때 기본값이 되므로 경고만 남긴다
	if (!std::isfinite(Value))
		HTR_LOG(Warning, "Save: non-finite float is written as null");
	Top() = Value;
}

void FJsonArchiveOutputFormatter::Serialize(double& Value)
{
	if (!std::isfinite(Value))
		HTR_LOG(Warning, "Save: non-finite double is written as null");
	Top() = Value;
}

void FJsonArchiveOutputFormatter::Serialize(FName& Value)
{
	Top() = Value.ToString();          // 실행마다 바뀌는 인덱스 대신 문자열
}

void FJsonArchiveOutputFormatter::Serialize(UObject*& Value)
{
	// 패키지가 없으므로 에셋 경로가 참조의 식별자 역할을 한다
	URenderAsset* Asset = Cast<URenderAsset>(Value);
	if (Asset && !Asset->GetPath().empty())
		Top() = Asset->GetPath();
	else
		Top() = nullptr;               // 경로 없는 객체는 되찾을 수 없음
}

void FJsonArchiveOutputFormatter::Serialize(void* Data, uint64 DataSize)
{
	json Bytes = json::array();
	const uint8* Src = static_cast<const uint8*>(Data);
	for (uint64 i = 0; i < DataSize; ++i)
		Bytes.push_back(Src[i]);
	Top() = std::move(Bytes);
}