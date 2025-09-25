// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FL_Generic.generated.h"


UCLASS()
class UFL_Generic : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Generic|UUID")
	static FString GenerateUUID();


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Generic|UUID")
	static void CalculateChunkCoordinatesAtWorldLocation(const FVector& WorldLocation, int64& ChunkX, int64& ChunkY, int64& ChunkZ);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Voxel|Coordinates")
	static void CalculateVoxelCoordinatesAtWorldLocation(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "utilities")
	static bool DeprojectScreenCenterToWorldPoint(APlayerController* PlayerController, float Distance, FVector& OutWorldPoint);
};
