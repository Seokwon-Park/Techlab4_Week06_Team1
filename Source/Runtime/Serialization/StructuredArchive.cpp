#include "EnginePCH.h"

#include "StructuredArchiveSlots.h"

FStructuredArchiveSlot FStructuredArchive::Open()
{
	assert(CurrentScope.Num() == 0);
	assert(!RootElementId.IsValid());
	assert(!CurrentSlotElementId.IsValid());

	RootElementId = ElementIdGenerator.Generate();
	CurrentScope.Emplace(RootElementId, EElementType::Root);
	CurrentSlotElementId = ElementIdGenerator.Generate();

	return FSlot(*this, 0, CurrentSlotElementId);
}

void FStructuredArchive::Close()
{
	if (CurrentScope.Num() == 0)
		return;
	SetScope(FSlot(*this, 0, RootElementId));
	CurrentScope.RemoveLast();          // Root 제거 → Close를 다시 불러도 안전
}

void FStructuredArchive::EnterSlot(const FSlotBase& Slot, bool bEnteringAttributedValue)
{
	int32 ParentDepth = Slot.Depth;
	FElementId ElementId = Slot.ElementId;

	if (ParentDepth + 1 < CurrentScope.Num() && CurrentScope[ParentDepth + 1].Id == ElementId && CurrentScope[ParentDepth + 1].Type == EElementType::AttributedValue)
	{
		SetScope(FSlot(*this, ParentDepth + 1, ElementId));
		Formatter.EnterAttributedValueValue();
	}
	else if (!bEnteringAttributedValue && Formatter.TryEnterAttributedValueValue())
	{
		int32 NewDepth = EnterSlotAsType(FSlotBase(*this, ParentDepth, ElementId), EElementType::AttributedValue);
		assert(NewDepth == ParentDepth + 1);
		FElementId AttributedValueId = CurrentScope[NewDepth].Id;
		SetScope(FSlot(*this, NewDepth, AttributedValueId));
	}
	else
	{
		assert(ElementId == CurrentSlotElementId);
		CurrentSlotElementId.Reset();
	}
}

int32 FStructuredArchive::EnterSlotAsType(const FSlotBase& Slot, EElementType Type)
{
	EnterSlot(Slot, Type == EElementType::AttributedValue);

	int32 NewSlotDepth = Slot.Depth + 1;

	if (NewSlotDepth < CurrentScope.Num() && CurrentScope[NewSlotDepth].Type == EElementType::AttributedValue)
	{
		++NewSlotDepth;
	}

	CurrentScope.Emplace(Slot.ElementId, Type);
	return NewSlotDepth;
}

void FStructuredArchive::LeaveSlot()
{
	switch (CurrentScope.Top().Type)
	{
		/*case FStructuredArchive::EElementType::Root:
			break;*/
	case EElementType::Record:
		Formatter.LeaveField();
		break;
	case EElementType::Array:
		Formatter.LeaveArrayElement();
		break;
	case EElementType::Stream:
		Formatter.LeaveStreamElement();
		break;
	case EElementType::Map:
		Formatter.LeaveMapElement();
		break;
	case EElementType::AttributedValue:
		Formatter.LeaveAttribute();
		break;
	default:
		break;
	}
}

void FStructuredArchive::SetScope(const FSlotBase& Slot)
{
	assert(Slot.Depth < CurrentScope.Num() && CurrentScope[Slot.Depth].Id == Slot.ElementId);
	assert(!CurrentSlotElementId.IsValid() || GetUnderlyingArchive().IsLoading());

	for (int32 CurrentDepth = CurrentScope.Num() - 1; CurrentDepth > Slot.Depth; CurrentDepth--)
	{
		// Leave the current element
		const FElement& Element = CurrentScope[CurrentDepth];
		switch (Element.Type)
		{
		case EElementType::Record:
			Formatter.LeaveRecord();
		case EElementType::Array:
			Formatter.LeaveArray();
			break;
		case EElementType::Stream:
			Formatter.LeaveStream();
			break;
		case EElementType::Map:
			Formatter.LeaveMap();
			break;
		case EElementType::AttributedValue:
			Formatter.LeaveAttributedValue();
			break;
		}

		CurrentScope.RemoveAt(CurrentDepth);

		LeaveSlot();
	}
	//CurrentScope.RemoveAt(Slot.Depth + 1, CurrentScope.Num() - (Slot.Depth + 1));

}

FArchive& FSlotBase::GetUnderlyingArchive() const
{
	return StructuredArchive.GetUnderlyingArchive();
}
