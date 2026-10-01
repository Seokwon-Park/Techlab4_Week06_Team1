#include "EnginePCH.h"
#include "ObjectFactory.h"
#include "Class.h"

//UObject* FObjectFactory::NewObject(UClass* Class, UObject* Outer, FName Name)
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
//    HTR_LOG(Info, "Create {}", Class->Name);
//    //HTR_LOG(Info, "Total Allocation Bytes - {}", FEngineStatics::TotalAllocationBytes);
//    //HTR_LOG(Info, "Total Allocation Count - {}", FEngineStatics::TotalAllocationCount);
//
//    return Object;
//}

UObject* FObjectFactory::ConstructObject(UClass* Class, UObject* Outer, FName Name)
{
    if (!Class || !Class->Constructor)
        return nullptr;
    
    UObject* Object = Class->Constructor();
    Object->ClassPrivate = Class;
    HashObject(Object, Class);

    Object->SetOuter(Outer);

    Name = MakeUniqueObjectName( Class, Outer, Name);

    Object->SetName(Name);

    //HTR_LOG(Info, "Create {}", Class->Name);
    //HTR_LOG(Info, "Total Allocation Bytes - {}", FEngineStatics::TotalAllocationBytes);
    //HTR_LOG(Info, "Total Allocation Count - {}", FEngineStatics::TotalAllocationCount);

    return Object;
}

FName FObjectFactory::MakeUniqueObjectName(const UClass* Class, UObject* Outer, FName BaseName)
{
    if (!Class)
        return FName();

    // 이름을 따로 안 줬으면 Class 이름을 기본 이름으로 사용
    if (BaseName == NAME_None)
    {
        BaseName = FName(Class->Name);
    }

    // 전체 UObject를 뒤지지 않고 Outer별 카운터로 번호를 매긴다.
    static TMap<FString, int32> RootNameCounters;   // Outer가 없는 오브젝트용
    TMap<FString, int32>& Counters = Outer ? Outer->ChildNameCounters : RootNameCounters;

    const FString Base = BaseName.ToString();
    int32& Next = Counters[Base];
    return FName(Base + "_" + std::to_string(Next++));
}
