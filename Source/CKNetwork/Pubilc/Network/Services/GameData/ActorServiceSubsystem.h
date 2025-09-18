// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CKTypes/Public/Shared/Types/Structures/Actors/FActorState.h"
#include "CKTypes/Public/Shared/Types/Structures/Actors/FActorUpdateStruct.h"
#include "CKPlayer/Public/Player/NonAuthClients/NPC_Manager.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "ActorServiceSubsystem.generated.h"

class UGameSessionSubsystem;
class UUDPSubsystem;
class UNetworkMessageParser;

/**
 * This Class Handles Actor Logics and dispatches requests, processes updates and hands over to NPC_Manager
 */
UCLASS(BlueprintType)
class CROWDEDKINGDOMS_API UActorServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, Category = "Actor Service Subsystem")
	void SendActorUpdate(const int64 ChunkX, const  int64 ChunkY, const int64 ChunkZ, const FString UUID, const FActorState& State);
	
	void HandleActorUpdateNotification(const TArray<uint8>& Payload) const;

	void HandleActorUpdateResponse(const TArray<uint8>& Payload) const;

	// Callsite: GameInstance after all subsystems have been initialized. This is to set the references
	UFUNCTION(BlueprintCallable, Category = "Actor Service Subsystem")
	virtual void PostSubsystemInit() override;

	UFUNCTION(BlueprintCallable, Category = "Actor Service Subsystem")
	void SetNPCManager(ANPC_Manager* InNPCManager){NPC_Manager = InNPCManager;}
	
private:
	
	void ProcessActorUpdateInfo(const FActorUpdateStruct& UpdateInfo) const;
	
	FCriticalSection ActorUpdateLock;

	UPROPERTY()
	ANPC_Manager* NPC_Manager;

	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;
	
	// Tracking Data
	UPROPERTY()
	uint32 CurrentSequenceNumber = 0;
	
	TMap<FString, TPair<uint32, double>> LastReceivedSequences;
	
};
