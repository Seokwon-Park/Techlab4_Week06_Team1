#pragma once
#include "Core/Types.h"

// Forward declarationss
struct FBox;
struct FVector;

//
// Magin numbers for numerical precision
//
#define INDEX_NONE -1 

#define THRESH_NORMALS_ARE_PARALLEL		(0.999845f)
#define SMALL_NUMBER					(1.e-8f)
#define KINDA_SMALL_NUMBER				(1.e-4f)

struct FMathUtil
{
	static bool LineExtentBoxIntersection(const FBox& InBox, const FVector& Start, const FVector& End, const FVector& Extent, FVector& HitLocation, FVector& HitNormal, float& HitTime);
};