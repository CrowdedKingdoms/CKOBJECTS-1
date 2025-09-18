// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "CKTypes/Public/Shared/Types/Structures/Chunks/FChunkDataContainer.h"
#include "ChunkDataManager.generated.h"

class UCDNServiceSubsystem;
class UChunkServiceSubsystem;
class UVoxelServiceSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogChunkLoader, Log, All);

struct FChunkDataState
{
	FChunkDataContainer GetChunkData;
	FChunkDataContainer CDNChunkData;
	FChunkDataContainer VoxelUpdates;
	
	bool bCDNResponseReceived = false;
	bool bChunkGetResponseReceived = false;
	bool bVoxelListResponseReceived = false;

	bool bCDNDataDirty = false;
	bool bGetChunkDataDirty = false;
	bool bVoxelListDataDirty = false;

	int32 LastCDNUpdateTime = 0;
	int32 LastGetChunkUpdateTime = 0;
	int32 LastVoxelListUpdateTime = 0;

	bool bProcessed = false;
};

UCLASS(Blueprintable, BlueprintType)
class UChunkDataManager : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Chunk Data Manager")
	virtual void PostSubsystemInit() override;

	UFUNCTION()
	void EnqueueChunksForRequesting(const TArray<FInt64Vector>& ChunkCoordinates);

	UFUNCTION(BlueprintCallable, Category = "Chunk Data Manager")
	void LoadInitialChunks();

	UFUNCTION()
	void UnloadFarOffChunksData(const TArray<FInt64Vector>& ChunkCoordinates);

	UFUNCTION()
	void UnloadAllChunkData();

	UFUNCTION()
	void SetNewCenterCoordinate(const FInt64Vector& NewCenter){CurrentCenterChunkCoordinate = NewCenter;}

	UFUNCTION()
	void OnCDNDataReceived(bool bSuccess, const FInt64Vector& ChunkCoordinate, const FChunkDataContainer& CDNData);

	UFUNCTION()
	void OnGetChunkDataReceived(bool bSuccess, TArray<FChunkDataContainer>& AllChunkData);

	UFUNCTION()
	void OnVoxelListDataReceived(bool bSuccess, const FInt64Vector& ChunkCoordinate, const FChunkDataContainer& VoxelListData);
	
private:

	UFUNCTION()
	void DequeueChunksForRequesting();

	UFUNCTION()
	bool IsChunkDirty(const FInt64Vector& ChunkCoordinate) const;

	UFUNCTION()
	TArray<FInt64Vector> GetDirtyChunks() const;

	UFUNCTION()
	void ProcessDirtyChunks();

	UFUNCTION()
	void ProcessDirtyChunksBatch();

	UFUNCTION()
	static void ApplyVoxelUpdates(FChunkDataContainer& ChunkData, const FChunkDataContainer& VoxelUpdates);

	static bool HasDataChanged(const FChunkDataContainer& OldData, const FChunkDataContainer& NewData);

	
	TQueue<FInt64Vector, EQueueMode::Mpsc> DirtyChunksQueue;
	
	UPROPERTY()
	FInt64Vector CurrentCenterChunkCoordinate {0, 0, 0};

	
	TQueue<FInt64Vector> ChunksToRequest;

	UPROPERTY()
	TMap<FInt64Vector, int32> RequestedChunks;
	
	
	TMap<FInt64Vector, FChunkDataState> LoadedChunks;

	UPROPERTY()
	UChunkServiceSubsystem* ChunkServiceSubsystem;

	UPROPERTY()
	UVoxelServiceSubsystem* VoxelServiceSubsystem;

	UPROPERTY()
	UCDNServiceSubsystem* CDNServiceSubsystem;

	UPROPERTY()
	int32 MaxBatchSize = 8;

	UPROPERTY()
	float TickInterval = 0.016f;

	UPROPERTY()
	TArray<FChunkDataContainer> ProcessingBatch;

	mutable FCriticalSection DataLock;
	mutable FCriticalSection ProcessingLock;

	FThreadSafeBool bIsTicking;
	FThreadSafeBool bShouldShutdown;

	UE::Tasks::FTask ProcessingTask;

	// Track the processing state better
	TSet<FInt64Vector> CurrentlyProcessing;

	
};