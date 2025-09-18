// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "Shared/Types/Structures/Voxels/FChunkVoxelState.h"
#include "Shared/Types/Structures/Voxels/FVoxelListItem.h"
#include "UObject/Object.h"
#include "Network/Services/GameData/ChunkServiceSubsystem.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Network/Services/GameData/CDNServiceSubsystem.h"
#include "Shared/Types/Structures/Chunks/FChunkDataContainer.h"
#include "VoxelDataManager.generated.h"


struct FVoxelDataState
{
	FChunkDataContainer GetChunkData;
	FChunkDataContainer CDNChunkData;
	FChunkDataContainer VoxelUpdates;
	
	bool bCDNResponseReceived = false;
	bool bChunkGetResponseReceived = false;
	bool bVoxelListResponseReceived = false;
};


/**
 *  This class handles the chunk voxel data receiving and processing logic.
 */
UCLASS(blueprintable, BlueprintType)
class CROWDEDKINGDOMS_API UVoxelDataManager : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Voxel Data Manager")
	virtual void PostSubsystemInit() override;
	
	void OnCDNResponseReceived(bool Success, int64 X, int64 Y, int64 Z, const TArray<uint8>& VoxelData, const TArray<FChunkVoxelState>& VoxelStates, int32 LastModifiedTimestamp);

	void ProcessVoxelData(int64 X, int64 Y, int64 Z);

	void OnGetChunkResponse(const bool Success, const TArray<FChunkDataContainer>& AllChunkData);
	void OnVoxelListResponse(const bool Success, const int64 X, const int64 Y, const int64 Z, const TArray<FChunkVoxelState>& ChunkVoxelStates);

	void AddVoxelCoordinatesToRequestList(const TArray<FInt64Vector>& VoxelCoords);

private:

	void CheckForVoxelData();

	void ProcessBatch();
	
	// Services References
	UPROPERTY()
	UCDNServiceSubsystem* CDNServiceSubsystem;
	
	UPROPERTY()
	UChunkServiceSubsystem* ChunkServiceSubsystem;

	UPROPERTY()
	UVoxelServiceSubsystem* VoxelServiceSubsystem;

	UPROPERTY()
	TArray<FChunkDataContainer> GetChunkData;

	UPROPERTY()
	TArray<FChunkDataContainer> FinalChunkDataArray;

	FThreadSafeBool bIsTicking;
	
	int32 CHUNK_SIZE = 16;

	// ThreadSafe Vars
	FCriticalSection DataLock;

	//Maps
	TMap<FInt64Vector, FVoxelDataState> PendingVoxelData;

	UPROPERTY()
	TArray<FInt64Vector> VoxelListCoordsToRequest;
};
