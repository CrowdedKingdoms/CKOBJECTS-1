// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "VoxelMeshProvider.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVoxelMeshProvider, Log, All);

UCLASS()
class UVoxelMeshProvider : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	UVoxelMeshProvider();

	// Simple mesh lookup functions
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	static void GetVoxelMesh(uint8 VoxelType, UProceduralMeshComponent* VoxelMesh, 
		UMaterialInstanceDynamic* MaterialInstance = nullptr, float VoxelSize = 100);
};
