#include "EnginePCH.h"
#include "JsonArchiveInputFormatter.h"

bool FJsonArchiveInputFormatter::HasDocumentTree() const
{
    return true;
}

void FJsonArchiveInputFormatter::EnterRecord()
{
    const json* Value = ValueStack.Top();
}

void FJsonArchiveInputFormatter::LeaveRecord()
{
}

void FJsonArchiveInputFormatter::EnterField(FArchiveFieldName Name)
{
}

void FJsonArchiveInputFormatter::LeaveField()
{
}

bool FJsonArchiveInputFormatter::TryEnterField(FArchiveFieldName Name, bool bEnterWhenSaving)
{
    return false;
}
