// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "CKTypes/Public/Shared/Types/Structures/Voxels/FVoxelListItem.h"
#include "Shared/Types/Structures/Voxels/FVoxelState.h"
#include "VoxelServiceSubsystem.generated.h"


class UChunkDataManager;
class UGraphQLService;
class UUDPSubsystem;
class UVoxelDataManager;
class UGameSessionSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogVoxelService, Log, All);

/**
 * This Class handles all logic related to Voxels i.e. VoxelWorldController. 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FOnVoxelUpdateNotify, int64, Cx, int64, Cy, int64, Cz, int32, Vx, int32, Vy, int32, Vz, uint8, voxelType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnVoxelUpdateResponse, int64, ChunkX, int64, ChunkY, int64, ChunkZ, int32, Vx, int32, Vy, int32, Vz);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnVoxelList, bool, bSuccess, int64, Cx, int64, Cy, int64, Cz, const TArray<FVoxelListItem>&, UpdatedVoxels);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_NineParams(FOnNewVoxelUpdateNotify, int64, Cx, int64, Cy, int64, Cz, int32, Vx, int32, Vy, int32, Vz, uint8, VoxelType, FVoxelState, VoxelState, bool, bHasState);

UCLASS(Blueprintable, BlueprintType)
class UVoxelServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Voxel Service Subsystem")
	virtual void PostSubsystemInit() override;
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Service")
	void SendVoxelListRequest(int64 X, int64 Y, int64 Z, int32 UnixTimestamp);

	UFUNCTION(BlueprintCallable, Category = "Voxel Service")
	void SendVoxelStateUpdateRequest(const int64 Cx, const int64 Cy, const int64 Cz, const int32 Vx, const int32 Vy, const int32 Vz, uint8 VoxelType, const FVoxelState VoxelState, const bool
	                                 bSendState);
	
	void HandleVoxelUpdateResponse(const TArray<uint8>& Payload) const;
	void HandleNewVoxelUpdateNotification(const TArray<uint8>& Payload) const;
	void HandleNewVoxelListResponse(const TArray<uint8>& Payload);

	void HandleVoxelListGraphQLResponse(const TSharedPtr<FJsonObject>& Payload) const;
	
	UPROPERTY(BlueprintAssignable, Category = "Voxel Service")
	FOnVoxelUpdateResponse OnVoxelUpdateResponse;

	UPROPERTY(BlueprintAssignable, Category = "Voxel Service")
	FOnNewVoxelUpdateNotify OnNewVoxelUpdateNotification;
	
private:

	UPROPERTY()
	UVoxelDataManager* VoxelDataManager;

	UPROPERTY()
	UChunkDataManager* ChunkDataManager;

	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;

	UPROPERTY()
	UGraphQLService* GraphQLService;
};