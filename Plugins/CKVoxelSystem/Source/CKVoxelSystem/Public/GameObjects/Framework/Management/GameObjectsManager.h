// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shared/Types/Structures/Events/FBaseEventState.h"
#include "Shared/Types/Structures/GameObjects/FGameObjectState.h"
#include "Shared/Types/Enums/Events/EEventType.h"
#include "GameObjectsManager.generated.h"


class UGameObjectsServiceSubsystem;
class AActivatorBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameObjectActivationNotificationReceived, const FString&, ObjectUUID, const FGameObjectState&, NewState);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBallEventReceived, const FString&, const FBallState&);

UCLASS(Blueprintable, BlueprintType)
class  AGameObjectsManager : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	AGameObjectsManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void DispatchActivationRequest(const int64 MapId, const int64 ChunkX, const int64 ChunkY,
	const int64 ChunkZ, const FString& ActivatorUUID, const uint16 EventType, const FGameObjectState& State) const;
	
	bool DoesEventExist(const FString& EventUUID) const;

	void InitializeEventMaps();

	UFUNCTION(BlueprintCallable, Category = "GameObjectsManager")
	void SetGameObjectsServiceReference(UGameObjectsServiceSubsystem* InGOService);

	void ProcessBallEvent(const FString& EventUUID, const FBallState& BallState) const;

	// Blueprint Only
	UFUNCTION(BlueprintCallable, Category = "GameObjectsManager")
	void SpawnBall(FBallState BallState);

	// C++ Only
	void SpawnBall(const FBallState& BallState) const;
	
	FOnGameObjectActivationNotificationReceived OnGameObjectActivationNotificationReceived;
	FOnBallEventReceived OnBallEventReceived;

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "GameObjectsManager")
	void FindAndActivateObject(const FString& ObjectUUID, const FGameObjectState& NewState);
	
	UPROPERTY(BlueprintReadWrite, Category = "GameObjectsManager")
	TMap<FString, AActivatorBase*> ActivatorsMap;

	UPROPERTY(BlueprintReadWrite, Category = "GameObjectsManager")
	UGameObjectsServiceSubsystem* GameObjectService;
	
	UPROPERTY(BlueprintReadWrite, Category = "GameObjectsManager")
	TMap<FString, UClass*> FixedEventsMap;

	TMap<EEventType, TSubclassOf<AActor>> EventActorClassMap;

	TMap<FString, EEventType> EventTypeMap; 

};
