// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Audio/VoiceChat/VoiceChatWorldSubsystem.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "opus.h"
#include "Network/Services/Core/UDPSubsystem.h"
#include "VoiceChatServiceSubsystem.generated.h"


class UGameSessionSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogVoiceService, Log, All);

/**
 * Handles Logic for Voice Chat Service
 */

UCLASS(BLueprintable, BlueprintType)
class   UVoiceChatServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:
	//Constructor and Destructor

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Voice Chat Service Subsystem")
	virtual void PostSubsystemInit() override;
	
	//Compression
	UFUNCTION()
	void CompressAudioData(const TArray<float>& InAudioData, int32 SampleRate, int32 NumChannels);

	//Decompression 
	bool DecompressAudioData(OpusDecoder* DecoderToUse, const TArray<uint8>& CompressedData, TArray<float>& OutAudioData, int32 SampleRate, int32 Channels);

	// Send Function
	void SendAudioData(const TArray<uint8>& InAudioData, int32 EncodedBytes, const int32 SampleRate, const int32 NumChannels);

	// Handler for Incoming Audio Data
	void HandleClientAudioNotification(const TArray<uint8>& Payload);

	// Called from the LEVEL_WORLD to set reference and bind event.
	UFUNCTION(BlueprintCallable, Category = "Voice Chat Service")
	void SetVoiceChatManager(UVoiceChatWorldSubsystem* InVoiceChatManager)
	{
		VoiceChatManager = InVoiceChatManager;
		if (VoiceChatManager)
		{
			UE_LOG(LogVoiceService, Log, TEXT("Voice Chat Manager Set"));
		}
		VoiceChatManager->OnAudioDataGenerated.AddDynamic(this, &UVoiceChatServiceSubsystem::CompressAudioData);
		VoiceChatManager->OnStreamTimeout.AddDynamic(this, &UVoiceChatServiceSubsystem::CleanupDecoder);
	}

	void SetUDPService(UUDPSubsystem* InUDPService);

	UPROPERTY(BlueprintReadWrite, Category="Voice Chat Service")
	bool bOwnerEcho = false;

private:

	// Opus Functions
	void InitializeOpusEncoder(int32 SampleRate, int32 Channels);
	OpusDecoder* GetOrCreateDecoder(const FString& UUID, int32 SampleRate, int32 Channels);

	UFUNCTION()
	void CleanupDecoder(const FString& UUID);
	void CleanupAllDecoders();
	
	void CleanupOpus();

	// Opus Vars
	OpusEncoder* AudioEncoder;
	OpusDecoder* AudioDecoder;

	TMap<FString, OpusDecoder*> UUIDToDecoderMap;

	// Chat Manager References
	UPROPERTY()
	UVoiceChatWorldSubsystem* VoiceChatManager;

	// Syncing And Threading
	FCriticalSection AudioLock;

	// Sending Payload Related vars
	TArray<uint8> AccumulatedPayload;
	const int32 MAX_PAYLOAD_SIZE = 4096;
	const int32 MIN_PAYLOAD_THRESHOLD = 212; // To ensure total packet size does not exceed 256 Bytes

	// Timestamp Calculation for Sending Function 
	double LastSendTimestamp = 0.0f;
	const double MAX_AGGREGATION_TIME = 0.1f;
	

	//Send Header
	bool bHeaderWritten = false;

	// Audio Params
	int32 CurrentSampleRate = 0;
	int32 CurrentNumChannels = 0;
	const float CAPTURE_DURATION = 0.02f; // 20ms
	
	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;

	UPROPERTY()
	int32 FrameCount;
	
};
