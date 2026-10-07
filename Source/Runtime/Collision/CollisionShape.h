#pragma once
#include "Math/Vector.h"

namespace ECollisionShape
{
	enum Type
	{
		Line,
		Box,
		Sphere,
		Capsule,
	};
};

struct FCollisionShape
{
	ECollisionShape::Type ShapeType;
	
	static constexpr float MinBoxExtent() { return KINDA_SMALL_NUMBER; }
	static constexpr float MinSphereRadius() { return KINDA_SMALL_NUMBER; }
	static constexpr float MinCapsuleRadius() { return KINDA_SMALL_NUMBER; }
	
	union
	{
		struct
		{
			float HalfExtentX;
			float HalfExtentY;
			float HalfExtentZ;
		} Box;

		struct
		{
			float Raidus;
		} Sphere;

		struct
		{
			float Radius;
			float HalfHeight;
		} Capsule;
	};

	FCollisionShape()
	{
		ShapeType = ECollisionShape::Line;
	}

	bool IsLine() const
	{
		return ShapeType == ECollisionShape::Line;
	}

	bool IsBox()const
	{
		return ShapeType == ECollisionShape::Box;
	}

	bool IsSphere() const
	{
		return ShapeType == ECollisionShape::Sphere;
	}

	bool IsCapsule() const
	{
		return ShapeType == ECollisionShape::Capsule;
	}

	void SetBox(const FVector& HalfExtent)
	{
		ShapeType = ECollisionShape::Box;
		Box.HalfExtentX = HalfExtent.X;
		Box.HalfExtentY = HalfExtent.Y;
		Box.HalfExtentZ = HalfExtent.Z;
	}

	void SetSphere(const float Radius)
	{
		ShapeType = ECollisionShape::Sphere;
		Sphere.Raidus = Radius;
	}

	void SetCapsule(const float Radius, const float HalfHeight)
	{
		ShapeType = ECollisionShape::Capsule;
		Capsule.Radius = Radius;
		Capsule.HalfHeight = HalfHeight;
	}

	void SetShape(const ECollisionShape::Type InShapeType, const FVector& Extent)
	{
		switch (InShapeType)
		{
		case ECollisionShape::Box:
		{
			SetBox(Extent);
		}
		break;
		case ECollisionShape::Sphere:
		{
			SetSphere(Extent.X);		
		}
		break;
		case ECollisionShape::Capsule:
		{
			SetCapsule(std::max(Extent.X,Extent.Y), Extent.Z);
		}
		break;
		case ECollisionShape::Line:
		default:
			ShapeType = InShapeType;
		}
	}

	bool IsNearlyZero() const
	{
		switch (ShapeType)
		{					
		case ECollisionShape::Box:
		{
			return(Box.HalfExtentX <= FCollisionShape::MinBoxExtent() && Box.HalfExtentY <= FCollisionShape::MinBoxExtent() && Box.HalfExtentZ <= FCollisionShape::MinBoxExtent());
		}
		case ECollisionShape::Sphere:
		{
			return (Sphere.Raidus <= FCollisionShape::MinSphereRadius());
		}
		case ECollisionShape::Capsule:
		{
			return (Capsule.Radius <= FCollisionShape::MinCapsuleRadius());
		}		
		}

		return true;
	}

	FVector GetExtent() const
	{
		switch (ShapeType)
		{		
		case ECollisionShape::Box:
		{
			return FVector(Box.HalfExtentX, Box.HalfExtentY, Box.HalfExtentZ);
		}
		case ECollisionShape::Sphere:
		{
			return FVector(Sphere.Raidus, Sphere.Raidus, Sphere.Raidus);
		}
		case ECollisionShape::Capsule:
		{
			return FVector(Capsule.Radius, Capsule.Radius, Capsule.HalfHeight);
		}
		}

		return FVector::ZeroVector;
	}

	FVector GetBox() const
	{
		return FVector(Box.HalfExtentX, Box.HalfExtentY, Box.HalfExtentZ);
	}

	const float GetSphereRadius() const
	{
		return Sphere.Raidus;
	}

	const float GetCapsuleRadius() const
	{
		return Capsule.Radius;
	}

	const float GetCapsuleHalfHeight() const
	{
		return Capsule.HalfHeight;
	}

	static FCollisionShape MakeBox(const FVector& BoxHalfExtent)
	{
		FCollisionShape BoxShpe;
		BoxShpe.SetBox(BoxHalfExtent);
		return BoxShpe;
	}

	static FCollisionShape MakeSphere(const float SphereRaidus)
	{
		FCollisionShape SphereShape;
		SphereShape.SetSphere(SphereRaidus);
		return SphereShape;
	}
};