#pragma once

#include "CoreMinimal.h"
#include "FStructureTemplate.generated.h"


USTRUCT(BlueprintType)
struct FStructureTemplate
{
	GENERATED_BODY()

	//UPROPERTY()
	//TArray<FVoxelData> VoxelPattern;

	UPROPERTY()
	FIntVector Size;

	UPROPERTY()
	FString StructureName;

	UPROPERTY()
	float SpawnProbability;

	UPROPERTY()
	TArray<int32> RequiredBiomes;
	
};