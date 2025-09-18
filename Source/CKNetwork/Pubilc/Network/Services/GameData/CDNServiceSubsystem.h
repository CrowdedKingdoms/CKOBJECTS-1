// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "CDNServiceSubsystem.generated.h"

class UChunkDataManager;
class UNetworkMessageParser;
DECLARE_LOG_CATEGORY_EXTERN(LogCDNService, Log, All);

class UChunkServiceSubsystem;
class UVoxelServiceSubsystem;
class UVoxelDataManager;
class UGameSessionSubsystem;

/**
 * This Class Handles CDN Logic
 */


USTRUCT(BlueprintType)
struct FCDNStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CDN Stats")
	mutable int32 CDNRequestsPerSecond = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CDN Stats")
	mutable int32 CDNResponsePerSecond = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CDN Stats")
	mutable int32 CDNOutgoingBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CDN Stats")
	mutable int32 CDNIncomingBytes = 0;
	
};

UCLASS(Blueprintable, BlueprintType)
class UCDNServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "CDN Service")
	virtual void PostSubsystemInit() override;
	
	UFUNCTION(BlueprintCallable, Category = "CDN Service")
	bool GetChunkCDN(int64 X, int64 Y, int64 Z);
	
	UFUNCTION(BlueprintCallable, Category = "CDN Service|Stats")
	FCDNStats GetCDNStats() const;

	void StartCDNStatsTimer();

	UFUNCTION(BlueprintCallable, Category = "CDN Service")
	void SetCDNEndpoint(FString Endpoint) 
	{ 
		CDN_Endpoint = Endpoint;
		UE_LOG(LogTemp, Log, TEXT("CDN Endpoint set to %s"), *CDN_Endpoint);
	}
	
protected:
	
	void UpdateCDNStats();
	

private:

	void OnCDNResponseReceived(FHttpRequestPtr Request,  FHttpResponsePtr Response, bool bWasSuccessful);

	UPROPERTY()
	FString CDN_Endpoint;
	
	UPROPERTY()
	UChunkServiceSubsystem* ChunkServiceSubsystem;

	UPROPERTY()
	UVoxelServiceSubsystem* VoxelServiceSubsystem;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;

	UPROPERTY()
	UVoxelDataManager* VoxelDataManager;

	UPROPERTY()
	UChunkDataManager* ChunkDataManager;
	
	UPROPERTY()
	int32 PreviousTimestamp;

	UPROPERTY()
	FTimerHandle CDNStatsTimerHandle;

	UPROPERTY()
	FCDNStats CDNStats;
	
	FThreadSafeCounter CDNRequestsPerSecond = 0;
	FThreadSafeCounter CDNResponsePerSecond = 0;
	FThreadSafeCounter CDNOutgoingBytes = 0;
	FThreadSafeCounter CDNIncomingBytes = 0;
};
