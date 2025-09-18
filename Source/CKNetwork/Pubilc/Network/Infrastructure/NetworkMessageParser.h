// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "UObject/Object.h"
#include "NetworkMessageParser.generated.h"

class UMessageBufferPoolSubsystem;
class UActorServiceSubsystem;
class UChunkServiceSubsystem;
class UUserServiceSubsystem;
class UTextChatServiceSubsystem;
class UVoiceChatServiceSubsystem;
class UUDPSubsystem;
class UVoxelServiceSubsystem;
class UCDNServiceSubsystem;
class UGameObjectsServiceSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(NetworkMessageParser, Log, All);


/**
 * This Class Handles Networking Logic
 */

UCLASS(Blueprintable, BlueprintType)
class CROWDEDKINGDOMS_API UNetworkMessageParser : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION()
	void ParseMessage(TArray<uint8>& Message) const;
	
	// Message Related Functions -- END --

	UFUNCTION(BlueprintCallable, Category = "Network Manager")
	void SetBufferPool(UMessageBufferPoolSubsystem* InBufferPool){BufferPoolSubsystem = InBufferPool;}

	UFUNCTION(BlueprintCallable, Category = "Network Manager")
	virtual void PostSubsystemInit() override;
	
	
	
private:
	
	static constexpr uint32 HEADER_LEN = 5U;

	// Thread-safe vars
	FCriticalSection ParsingLock;
	
	//Services Required
	UPROPERTY()
	UMessageBufferPoolSubsystem* BufferPoolSubsystem;
	
	UPROPERTY()
	UUserServiceSubsystem* UserServiceSubsystem;

	UPROPERTY()
	UActorServiceSubsystem* ActorServiceSubsystem;

	UPROPERTY()
	UChunkServiceSubsystem* ChunkServiceSubsystem;

	UPROPERTY()
	UCDNServiceSubsystem* CDNServiceSubsystem;
	
	UPROPERTY()
	UVoxelServiceSubsystem* VoxelServiceSubsystem;

	UPROPERTY()
	UVoiceChatServiceSubsystem* VoiceChatServiceSubsystem;

	UPROPERTY()
	UTextChatServiceSubsystem* TextChatServiceSubsystem;

	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;

	UPROPERTY()
	UGameObjectsServiceSubsystem* GameObjectServiceSubsystem;

	
};
