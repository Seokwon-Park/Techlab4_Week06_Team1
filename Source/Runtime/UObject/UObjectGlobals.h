#pragma once

#include "Object.h"

struct FStaticConstructObjectParameters
{
	const UClass* Class = nullptr;
	UObject* Outer = nullptr;
	FName Name;

	FStaticConstructObjectParameters(const UClass* InClass);

	EObjectFlags SetFlags = EObjectFlags::RF_NoFlags;
};


UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters& Params);
FName MakeUniqueObjectName(UObject* Outer, const UClass* Class, FName BaseName = NAME_None);
UObject* StaticDuplicateObject(UObject const* SourceObject, UObject* DestOuter, const FName DestName = NAME_None);

// 언리얼이 UObject를 할당하는 방식. 
// 여기서 memset을 모두 0으로 만들어 주는 것으로 초기화 변수 누락을 방지
UObject* StaticAllocateObject(const UClass* InClass, UObject* InOuter, FName InName, EObjectFlags InFlags);

template <class T>
T* NewObject
(
	UObject* Outer, UClass* Class, FName Name = NAME_None, EObjectFlags Flags = EObjectFlags::RF_NoFlags
)
{
	FStaticConstructObjectParameters Params(Class);
	Params.Outer = Outer;
	Params.Name = Name;
	Params.SetFlags = Flags;

	T* Result = static_cast<T*>(StaticConstructObject_Internal(Params));
	return Result;
};

template <class T>
T* NewObject(UObject* Outer = nullptr)
{
	FStaticConstructObjectParameters Params(T::StaticClass());
	Params.Outer = Outer;

	T* Result = static_cast<T*>(StaticConstructObject_Internal(Params));
	return Result;
}

template <class T>
T* NewObject(UObject* Outer, FName Name, EObjectFlags Flags = EObjectFlags::RF_NoFlags)
{
	FStaticConstructObjectParameters Params(T::StaticClass());
	Params.Outer = Outer;
	Params.Name = Name;
	Params.SetFlags = Flags;

	T* Result = static_cast<T*>(StaticConstructObject_Internal(Params));
	return Result;
}




