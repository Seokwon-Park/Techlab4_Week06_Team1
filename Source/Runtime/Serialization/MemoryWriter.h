#pragma once

#include "Core/Types.h"
#include "MemoryArchive.h"

template <typename ArrayAllocatorType>
class TMemoryWriterBase : public FMemoryArchive
{
		using IndexSizeType = typename ArrayAllocatorType::SizeType;
		static constexpr int32 IndexSize = sizeof(IndexSizeType) * 8;

};