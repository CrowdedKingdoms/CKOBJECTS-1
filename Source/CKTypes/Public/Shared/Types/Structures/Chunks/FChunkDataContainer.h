#pragma once

#include "CoreMinimal.h"
#include "CKTypes/Public/Shared/Types/Structures/Voxels/FVoxelCoordinate.h"
#include "CKTypes/Public/Shared/Types/Structures/Voxels/FVoxelDefinition.h"
#include "FChunkDataContainer.generated.h"

USTRUCT(BlueprintType)
struct FChunkDataContainer
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY()
	FInt64Vector ChunkCoordinate;

	UPROPERTY()
	TArray<uint8> VoxelData;

	UPROPERTY()
	TMap<FVoxelCoordinate, FVoxelDefinition> VoxelStatesMap;
};
