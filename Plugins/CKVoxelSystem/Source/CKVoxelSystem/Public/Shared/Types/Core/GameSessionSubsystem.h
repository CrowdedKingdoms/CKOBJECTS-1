// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Enums/Network/MessageType.h"
#include "UObject/Object.h"
#include "Containers/Queue.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeCounter.h"
#include "GameSessionSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGraphUpdated);
/**
 * Graph contains various data structures that threads may access and update.
 */

UCLASS(Blueprintable, BlueprintType)
class  UGameSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	bool EnqueueMessageToSend(const TArray<uint8>& Message);

	void DequeueMessageToSend(TArray<uint8>& OutMessage);

	void EnqueueMessageToReceive(const TArray<uint8>& Message);

	bool DequeueMessageToReceive(TArray<uint8>& OutMessage);
	

	// Session Variables Setters
	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SetMapID(const int64 InMapID) {MapID = InMapID;}
	
	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SetUserID(const int64 InUserID){UserID = InUserID;}

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SetGameToken(const FString InGameToken){GameToken = InGameToken;}
	
	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SetUUID(FString InUUID){PlayerUUID = InUUID;}

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SetGameTokenID(const int64 InGameTokenID) {GameTokenID = InGameTokenID;}

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void SetPlayerCurrentChunkCoordinates(const int64 X, const int64 Y, const int64 Z) {CurrentPlayerChunkCoordinates = FInt64Vector(X, Y, Z);}

	UFUNCTION(Category = "Graph")
	FInt64Vector GetPlayerCurrentChunkCoordinates() const {return CurrentPlayerChunkCoordinates;}
	
	// Session Variables Getters
	UFUNCTION(BlueprintCallable, Category = "Graph")
	int64 GetMapID() const {return MapID;}
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Graph", meta=(CompactNodeTitle = "Game Token"))
	FString GetGameToken() const {return GameToken;}
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Graph", meta=(CompactNodeTitle = "Owning Player UUID"))
	FString GetUUID() const {return PlayerUUID;}

	UFUNCTION(BlueprintCallable, Category = "Graph", meta=(CompactNodeTitle = "User ID"))
	int64 GetUserID() const {return UserID;}

	UFUNCTION(BlueprintPure, Category = "Graph", meta=(CompactNodeTitle = "Game Token ID"))
	int64 GetGameTokenID() const {return GameTokenID;}

	// Helper Functions 
	UFUNCTION(BlueprintCallable, Category = "Graph")
	void ClearSessionData();

	UFUNCTION(BlueprintCallable, Category = "Graph")
	void PrintCounterValues() const;

	UFUNCTION()
	bool HasPendingIncomingMessages() const;

	UFUNCTION()
	bool HasPendingOutgoingMessages() const;

private:

	UPROPERTY()
	int64 MapID;

	UPROPERTY()
	FString GameToken;

	UPROPERTY()
	FString PlayerUUID;

	UPROPERTY()
	int64 UserID;
	
	UPROPERTY()
	int64 GameTokenID;

	UPROPERTY()
	FInt64Vector CurrentPlayerChunkCoordinates = {0, 0, 0};

	TQueue<TArray<uint8>, EQueueMode::Mpsc> SendQueue;
	TQueue<TArray<uint8>, EQueueMode::Spsc> ReceiveQueue;
	
	FThreadSafeCounter SendCounter;
	FThreadSafeCounter ReceiveCounter;
	
	FCriticalSection SendQueueMutex;
	FCriticalSection ReceiveQueueMutex;
};
