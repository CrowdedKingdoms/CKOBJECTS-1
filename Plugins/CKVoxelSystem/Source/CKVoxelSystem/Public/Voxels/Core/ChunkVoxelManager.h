// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shared/Types/Enums/Voxels/EVoxelType.h"
#include "ChunkVoxelManager.generated.h"

class AVoxelChunk;
class UVoxelWorldSubsystem;
class UVoxelServiceSubsystem;

UCLASS(Blueprintable, BlueprintType)
class  AChunkVoxelManager : public AActor
{
	GENERATED_BODY()

public:
	AChunkVoxelManager();
	
	UFUNCTION(BlueprintCallable, Category="ChunkVoxelManager")	
	void SetVoxelType(uint8 VoxelType);
	
	UFUNCTION(BlueprintCallable, Category="ChunkVoxelManager")
	void ChunkVoxelManager();

	UFUNCTION(BlueprintCallable, Category = "ChunkVoxelManager")
	void RemoveVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int32 Vx, int32 Vy, int32 Vz);

	UFUNCTION(BlueprintCallable, Category = "ChunkVoxelManager")
	void OnNewVoxelUpdate(int64 Cx, int64 Cy, int64 Cz, int32 Vx, int32 Vy, int32 Vz, uint8 VoxelType, FVoxelState VoxelState, bool bHasState);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager")
	EVoxelType TypeOfVoxel = EVoxelType::AIR;

	TWeakObjectPtr<AActor> MyOwner;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager")
	UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager")
	UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;

	
};
