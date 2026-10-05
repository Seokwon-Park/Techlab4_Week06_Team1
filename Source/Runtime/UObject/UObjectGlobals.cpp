#include "EnginePCH.h"
#include "UObjectGlobals.h"

#include "UObjectHash.h"

#include "Class.h"

//UObject* FObjectFactory::ConstructObject(UClass* Class, UObject* Outer, FName Name)
//{
//    if (!Class || !Class->Constructor)
//        return nullptr;
//
//    UObject* Object = Class->Constructor();
//    Object->ClassPrivate = Class;
//    HashObject(Object, Class);
//
//    Object->SetOuter(Outer);
//
//    Name = MakeUniqueObjectName(Class, Outer, Name);
//
//    Object->SetName(Name);
//
//    //HTR_LOG(Info, "Create {}", Class->Name);
//    //HTR_LOG(Info, "Total Allocation Bytes - {}", FEngineStatics::TotalAllocationBytes);
//    //HTR_LOG(Info, "Total Allocation Count - {}", FEngineStatics::TotalAllocationCount);
//
//    return Object;
//}

UObject* StaticConstructObject_Internal(const FStaticConstructObjectParameters& Params)
{
	const UClass* Class = Params.Class;
	if (!Class || !Class->Constructor)
		return nullptr;

	UObject* Object = Class->Constructor();
	Object->ClassPrivate = const_cast<UClass*>(Class);
	HashObject(Object, Class);

	Object->SetOuter(Params.Outer);
	Object->SetName(Params.Name);

	if (Params.SetFlags != EObjectFlags::RF_NoFlags)
		Object->SetFlags(Params.SetFlags);

	return Object;
}

FName MakeUniqueObjectName(UObject* Outer, const UClass* Class, FName BaseName)
{
	if (!Class)
		return FName();

	// 이름을 따로 안 줬으면 Class 이름을 기본 이름으로 사용
	if (BaseName == NAME_None)
	{
		BaseName = FName(Class->Name);
	}

	const FString Base = BaseName.ToString();
	return FName(Base + "_" + std::to_string(Class->ClassUnique++));
}

UObject* StaticAllocateObject(const UClass* InClass, UObject* InOuter, FName InName, EObjectFlags InFlags)
{
	void* Mem = malloc(InClass->ClassSize);
	UObject* Obj = nullptr;
	return nullptr;
}

FStaticConstructObjectParameters::FStaticConstructObjectParameters(const UClass* InClass)
	: Class(InClass)
{
}

