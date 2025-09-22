// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "Shared/Types/Structures/Voxels//FChunkVoxelState.h"
#include "Network/Services/GameData/CDNServiceSubsystem.h"
#include "ChunkServiceSubsystem.generated.h"

class UGameSessionSubsystem;
class UGraphQLService;
class UVoxelDataManager;

DECLARE_LOG_CATEGORY_EXTERN(LogChunkService, Log, All);

/**
 * This Class handles logic related to Chunk Service
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnChunkLoadedResponse, bool, bSuccess, int64, x, int64, y, int64, z, const TArray<uint8>&, voxels, const TArray<FChunkVoxelState>&, VoxelStates);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnChunkUpdatedResponse, bool, bSuccess, int64, X, int64, Y, int64, Z);

UCLASS(Blueprintable, BlueprintType)
class  UChunkServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Chunk Service Subsystem")
	virtual void PostSubsystemInit() override;
	
	UFUNCTION(BlueprintCallable, Category = "Chunk Service")
	void UpdateChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& Voxels);

	UFUNCTION(BlueprintCallable, Category = "Chunk Service")
	void GetChunkByDistance(int64 X, int64 Y, int64 Z, int32 MaxDistance, int32 Limit, int32 Skip);
	
	void HandleUpdateChunkResponse(const TSharedPtr<FJsonObject>& Payload) const;

	void HandleGetChunkResponse(const TSharedPtr<FJsonObject>& Payload) const;

	UPROPERTY(BlueprintAssignable, Category = "Chunk Service")
	FOnChunkLoadedResponse OnChunkLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Chunk Service")
	FOnChunkUpdatedResponse OnChunkUpdated;

private:

	UPROPERTY()
	UGraphQLService* GraphQLService;
	
	UPROPERTY()
	UCDNServiceSubsystem* CDNServiceSubsystem;

	UPROPERTY()
	UVoxelDataManager* VoxelDataManager;

	UPROPERTY()
	UChunkDataManager* ChunkDataManager;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;
};
