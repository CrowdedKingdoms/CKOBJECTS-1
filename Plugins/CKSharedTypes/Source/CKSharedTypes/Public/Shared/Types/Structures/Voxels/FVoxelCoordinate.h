#pragma once
#include "CoreMinimal.h"
#include "FVoxelCoordinate.generated.h"

USTRUCT()
struct FVoxelCoordinate
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY()
	uint8 X;

	UPROPERTY()
	uint8 Y;

	UPROPERTY()
	uint8 Z;

	FVoxelCoordinate()
		: X(0), Y(0), Z(0) {}

	FVoxelCoordinate(const uint8 inX, const uint8 inY, const uint8 inZ) : X(inX), Y(inY), Z(inZ) {}

	FORCEINLINE bool operator==(const FVoxelCoordinate& Other) const
	{
		return Version == Other.Version && X == Other.X && Y == Other.Y && Z == Other.Z;
	}
};

FORCEINLINE uint32 GetTypeHash(const FVoxelCoordinate& Coord)
{
	const uint32 Hash = FCrc::MemCrc32(&Coord, sizeof(FVoxelCoordinate));
	return Hash;
}