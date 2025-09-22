// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shared/Types/Structures/Actors/FActorState.h"
#include "GameFramework/Character.h"
#include "Shared/Types/Enums/Character/ECharacterType.h"
#include "Shared/Types/Interfaces/World/OriginRebasable.h"
#include "Shared/Types/Structures/Actors/FActorUpdateStruct.h"
#include "NPC_Manager.generated.h"

class UVoxelWorldSubsystem;
class ASkelotInstanceManager;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpawnNPC, FString, UUID, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUpdateNPC, FString, UUID, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActorTimeout, const FString&, UUID);

UCLASS()
class  ANPC_Manager : public AActor, public IOriginRebasable
{
	GENERATED_BODY()

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	ANPC_Manager();

	UFUNCTION(BlueprintCallable, Category = "NPC_Manager")
	void ProcessUpdates(const FActorUpdateStruct& UpdateInfo);
	
	
	// This is set in Blueprint on Begin Play.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC_Manager")
	FString OwnerUUID;

	// Function Delegate for Spawning Player
	UPROPERTY(BlueprintAssignable, Category = "NPC_Manager")
	FSpawnNPC SpawnNPC;

	// Function Delegate for Updates
	UPROPERTY(BlueprintAssignable, Category = "NPC_Manager")
	FUpdateNPC UpdateNPC;

	// Function Delegate for Actor Timeout
	UPROPERTY(BlueprintAssignable, Category = "NPC_Manager")
	FActorTimeout ActorTimeout;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPC_Manager")
	float ActorTimeoutThreshold = 2.0f;
	
	//CharacterType Enum to pass
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC_Manager")
	ECharacterType CharacterTypeToSpawn;

	UPROPERTY(BlueprintReadWrite, Category = "NPC_Manager")
	bool bPlayerGhostEnabled = false;

	UFUNCTION(BlueprintCallable, Category = "NPC_Manager")
	FVector GetActorOriginalLocation_Implementation() override;
private:
	
	//Actor Updates Timeout Tracking
	TMap<FString, float> LastUpdateTimes;
	FTimerHandle ActorTimeoutTimerHandle;

	FThreadSafeBool bShouldStopTimeoutTask = false;
	void StartTimeoutCheckingTask();
	void CheckForActorTimeouts();
	void UpdateLastSeenTime(const FString& UUID);

	//To Store Other Players' UUIDs
	UPROPERTY()
	TArray<FString> OtherPlayersUUID;

	//Storing ActorState for processing
	UPROPERTY()
	FActorState ReceivedActorState;

	//Function to check if incoming UUID is already in the list or not
    UFUNCTION(BlueprintCallable, Category = "NPC_Manager")
    bool CheckPlayerExists(const FString& IncomingUUID) const;

	UFUNCTION()
	void RegisterPlayerToMap(const FString& IncomingUUID);
	
	//For Spawning New Player
    UFUNCTION(BlueprintCallable, Category = "NPC_Manager")
    void SpawnNewPlayer(const FString& InComingUUID, const int64& ChunkX, const int64& ChunkY, const int64& ChunkZ, FActorState& ActorState);

	//For Updating an existing Player in the list
	UFUNCTION(BlueprintCallable, Category = "NPC_Manager")
	void UpdatePlayer(const FString& InComingUUID, const int64& ChunkX, const int64& ChunkY, const int64& ChunkZ, FActorState& ActorState) const;

	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager")
	void SetVoxelWorldController(UVoxelWorldSubsystem* WorldController);

	
	//Locks 
	FCriticalSection UUIDLock;
	FCriticalSection ActorStateLock;
	FCriticalSection TimeoutMapLock;

	void ProcessTimedOutActor(const TArray<FString>& TimedOutActors) const;
	float TimeoutBroadcastDelay = 0.5f;

	UPROPERTY()
	FVector OriginalLocation;

	UPROPERTY(EditAnywhere)
	ASkelotInstanceManager* SkelotInstanceManager;

	UPROPERTY()
	UVoxelWorldSubsystem* VoxelWorldController;
};
