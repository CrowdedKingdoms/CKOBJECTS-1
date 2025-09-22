// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "Shared/Types/Structures/GameObjects/FGameObjectState.h"
#include "Shared/Types/Structures/Events/FBaseEventState.h"
#include "GameObjectsServiceSubsystem.generated.h"

class AGameObjectsManager;
class UUDPSubsystem;
class UGameSessionSubsystem;


/**
 * Used to handling GameObject Requests and responses
 */
UCLASS(Blueprintable, BlueprintType)
class  UGameObjectsServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Game Objects Service Subsystem")
	virtual void PostSubsystemInit() override;
	
	void SendGameObjectActivationRequest(const int64 MapId, const int64 ChunkX, const int64 ChunkY,
	                                            const int64 ChunkZ, const FString& ActivatorUUID, const uint16 EventType, const FGameObjectState& State) const;
	
	UFUNCTION(BlueprintCallable, Category = "GameObject Service")
	void SendTriggerBallEventRequest(int64 ChunkX, int64 ChunkY, int64 ChunkZ, const FString InUUID, const FBallState BallState);

	void HandleGameEventNotification(const TArray<uint8>& Payload) const;
	
	void HandleGameObjectActivationNotification(const TArray<uint8>& Payload) const;

	void HandleTriggerBallEventNotification(const TArray<uint8>& Payload) const;
	
	UFUNCTION(BlueprintCallable, Category = "GameObject Service")
	void SetGameObjectsManager(AGameObjectsManager* InGameObjectsManager) {GameObjectsManager = InGameObjectsManager;}

	

private:
	
	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;

	UPROPERTY()
	AGameObjectsManager* GameObjectsManager;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;
	
};
