// Fill out your copyright notice in the Description page of Project Settings.
#include "Network/Services/Communication/VoiceChatServiceSubsystem.h"
#include "Network/Infrastructure/NetworkMessageParser.h"
#include "Network/Services/Core/UDPSubsystem.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"

DEFINE_LOG_CATEGORY(LogVoiceService);

void UVoiceChatServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AudioEncoder = nullptr;
	AudioDecoder = nullptr;
	VoiceChatManager = nullptr;
}

void UVoiceChatServiceSubsystem::Deinitialize()
{
	CleanupOpus();
	CleanupAllDecoders();
	Super::Deinitialize();
}

void UVoiceChatServiceSubsystem::PostSubsystemInit()
{
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	
	if (!UDPSubsystem || !GameSessionSubsystem)
	{
		UE_LOG(LogVoiceService, Error, TEXT("One or more subsystems are invalid."));
		return;
	}
	

	UE_LOG(LogVoiceService, Log, TEXT("Voice Chat Service Subsystem Initialized"));
}


void UVoiceChatServiceSubsystem::CompressAudioData(const TArray<float>& InAudioData, int32 SampleRate, int32 NumChannels)
{

	FScopeLock Lock(&AudioLock);

	// Comprehensive input validation
	if (InAudioData.Num() == 0 || InAudioData.Num() % 2 != 0)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Input audio data is empty or invalid."));
		return;
	}
	
	// Ensure encoder is initialized
    if (!AudioEncoder)
    {
        InitializeOpusEncoder(SampleRate, 1);
        
        if (!AudioEncoder)
        {
            UE_LOG(LogVoiceService, Error, TEXT("Failed to initialize Opus Encoder"));
            return;
        }
    }

	
	const int32 Frame_Size = SampleRate * 1 * CAPTURE_DURATION;
	const int32 MaxCompressedSize = Frame_Size * sizeof(float);
	const int32 TotalFrames = (InAudioData.Num() + Frame_Size - 1) / Frame_Size;

	for (int32 FrameIndex = 0; FrameIndex < TotalFrames; ++FrameIndex)
	{
		// Extract or pad input audio frame
		TArray<float> FrameData;
		FrameData.SetNumZeroed(Frame_Size);  // Pre-fill with zeros

		const int32 StartSample = FrameIndex * Frame_Size;
		const int32 CopySize = FMath::Min(Frame_Size, InAudioData.Num() - StartSample);
		FMemory::Memcpy(FrameData.GetData(), InAudioData.GetData() + StartSample, CopySize * sizeof(float));

		TArray<uint8> CompressedAudioData;
		CompressedAudioData.SetNumUninitialized(MaxCompressedSize);

		const int32 EncodedBytes = opus_encode_float(AudioEncoder, FrameData.GetData(), Frame_Size,
													 CompressedAudioData.GetData(), MaxCompressedSize);

		if (EncodedBytes <= 0)
		{
			const char* ErrorMessage = opus_strerror(EncodedBytes);
			UE_LOG(LogVoiceService, Error, TEXT("Opus encoding failed: %s"), UTF8_TO_TCHAR(ErrorMessage));
			return;
		}

		UE_LOG(LogVoiceService, Log, TEXT("Compressed audio data size: %d"), EncodedBytes);
		
		// Trim the compressed data to fixed size, for easier encode/decode
		CompressedAudioData.SetNum(EncodedBytes, EAllowShrinking::No);

		SendAudioData(CompressedAudioData, EncodedBytes, SampleRate, 1);
	}
	
}

bool UVoiceChatServiceSubsystem::DecompressAudioData(OpusDecoder* DecoderToUse, const TArray<uint8>& CompressedData, TArray<float>& OutAudioData, int32 SampleRate, int32 Channels)
{
	FScopeLock Lock(&AudioLock);

	if (!DecoderToUse)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Opus Decoder is null."));
		return false;
	}

	if (CompressedData.Num() <= 0)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Input audio data is empty."));
		return false;
	}

	// Must Match this from compression side and Capture side. Otherwise, won't work.
	const int32 FrameSize = 1 * SampleRate * CAPTURE_DURATION;
	//UE_LOG(LogVoiceService, Log, TEXT("Frame size Decompression: %d"), FrameSize);
	
	const int32 FloatBufferSize = FrameSize * 1;
	OutAudioData.SetNumUninitialized(FloatBufferSize);

	// Encoding
	const int32 DecodedSamples = opus_decode_float(
		DecoderToUse,
		CompressedData.GetData(),
		CompressedData.Num(),
		OutAudioData.GetData(),
		FrameSize,
		0);
	

	if (DecodedSamples <= 0)
	{
		const char* ErrorMsg = opus_strerror(DecodedSamples);
		UE_LOG(LogVoiceService, Error, TEXT("Opus decoding failed: %s"), UTF8_TO_TCHAR(ErrorMsg));
		return false;
	}

	//Resizing Float Buffer to actual decoded size. 
	const int32 ActualFloatSize = DecodedSamples * 1;
	OutAudioData.SetNum(ActualFloatSize);


	//UE_LOG(LogVoiceService, Log, TEXT("Decompression successful - Decoded samples: %d, Output Float size: %d bytes, OutAudioData[0]: %f"), DecodedSamples, OutAudioData.Num(), OutAudioData[0]);
	return true;	
}

void UVoiceChatServiceSubsystem::SendAudioData(const TArray<uint8>& InAudioData, const int32 EncodedBytes, const int32 SampleRate, const int32 NumChannels)
{
	FScopeLock Lock(&AudioLock);

	if(InAudioData.Num() <= 0)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Compressed Payload is empty. Cannot be sent."))
		return;
	}
	
	// Write header once
	if (!bHeaderWritten || SampleRate != CurrentSampleRate || NumChannels != CurrentNumChannels)
	{
		AccumulatedPayload.Reset();

		const int64 MapID = GameSessionSubsystem->GetMapID();
		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&MapID), sizeof(int64));

		const FInt64Vector ChunkCoords = GameSessionSubsystem->GetPlayerCurrentChunkCoordinates();

		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&ChunkCoords.X), sizeof(int64));
		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&ChunkCoords.Y), sizeof(int64));
		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&ChunkCoords.Z), sizeof(int64));
		
		// UUID: make sure it’s 32 bytes
		const FString UUID = GameSessionSubsystem->GetUUID();
		const FTCHARToUTF8 ConvertedUUID(*UUID);
		const uint8* UUIDPtr = reinterpret_cast<const uint8*>(ConvertedUUID.Get());
		check(ConvertedUUID.Length() == 32); // Make sure it's really 32 characters
		AccumulatedPayload.Append(UUIDPtr, 32);

		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&SampleRate), sizeof(SampleRate));
		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&NumChannels), sizeof(NumChannels));

		// Placeholder for FrameCount (we'll patch this just before sending)
		int32 PlaceholderFrameCount = 0;
		AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&PlaceholderFrameCount), sizeof(int32));

		FrameCount = 0;
		bHeaderWritten = true;
		CurrentSampleRate = SampleRate;
		CurrentNumChannels = NumChannels;
	}

	// Append audio data
	const int32 FrameSize = EncodedBytes;
	AccumulatedPayload.Append(reinterpret_cast<const uint8*>(&FrameSize), sizeof(FrameSize));
	AccumulatedPayload.Append(InAudioData);
	FrameCount++;

	// Check if we should send the payload
	double CurrentTime = FPlatformTime::Seconds();
	bool bShouldSend = 
		(AccumulatedPayload.Num() >= MAX_PAYLOAD_SIZE) ||  // Payload is full
		(AccumulatedPayload.Num() >= MIN_PAYLOAD_THRESHOLD);
	 //&&(CurrentTime - LastSendTimestamp) >= MAX_AGGREGATION_TIME);  // Meaningful size + time elapsed

	if (!bShouldSend)
	{
		//UE_LOG(LogVoiceService, Log, TEXT("Accumulating more data."));
		return;
	}

	FMemory::Memcpy(AccumulatedPayload.GetData() + 72, &FrameCount, sizeof(int32));  // Patch FrameCount (we wrote it as placeholder

	// Actually send the accumulated payload
	if (UDPSubsystem->QueueUDPMessage(EMessageType::CLIENT_AUDIO_PACKET, AccumulatedPayload))
	{
		UE_LOG(LogVoiceService, Log, TEXT("Sending audio packet. Size: %d bytes"), AccumulatedPayload.Num());
		AccumulatedPayload.Reset();
		bHeaderWritten = false;
		LastSendTimestamp = CurrentTime;
		return;
	}

	
	UE_LOG(LogVoiceService, Error, TEXT("Failed to send Audio Packet"));
}

void UVoiceChatServiceSubsystem::HandleClientAudioNotification(const TArray<uint8>& Payload)
{
    // Validate minimum payload size (UUID [32] + SampleRate [4] + NumChannels [4])
    constexpr int32 MinHeaderSize = 32 + sizeof(int32) * 2;
    if (Payload.Num() < MinHeaderSize)
    {
        UE_LOG(LogVoiceService, Error, TEXT("Incomplete Audio Packet Size."));
        return;
    }
	
	int32 Offset = 0;

	// UUID (32-byte UTF-8 string, no null terminator)

	//int64 MapId = reinterpret_cast<const int64>(Payload.GetData() + Offset);
	Offset += sizeof(int64);

	//int64 ChunkX = reinterpret_cast<const int64>(Payload.GetData() + Offset);
	Offset += sizeof(int64);
	//int64 ChunkY = reinterpret_cast<const int64>(Payload.GetData() + Offset);
	Offset += sizeof(int64);
	//int64 ChunkZ = reinterpret_cast<const int64>(Payload.GetData() + Offset);
	Offset += sizeof(int64);
	
	
	const char* UUIDPtr = reinterpret_cast<const char*>(Payload.GetData() + Offset);
	const FUTF8ToTCHAR UTF8Converter(UUIDPtr, 32);
	const FString UUID(UTF8Converter.Length(), UTF8Converter.Get());
	Offset += 32;

    if (UUID.Equals(GameSessionSubsystem->GetUUID()))
    {
	    if (!bOwnerEcho)
	    {
		    return;
	    }
    }
	
	// SampleRate
	int32 SampleRate = 0;
	FMemory::Memcpy(&SampleRate, Payload.GetData() + Offset, sizeof(int32));
	Offset += sizeof(int32);

	// NumChannels
	int32 NumChannels = 0;
	FMemory::Memcpy(&NumChannels, Payload.GetData() + Offset, sizeof(int32));
	Offset += sizeof(int32);

	// Frame count
	int32 Lcl_FrameCount = 0;
	FMemory::Memcpy(&Lcl_FrameCount, Payload.GetData() + Offset, sizeof(int32));
	Offset += sizeof(int32);

	// Sanity check
	if (Lcl_FrameCount <= 0 || Lcl_FrameCount > 100)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Invalid frame count: %d"), Lcl_FrameCount);
		return;
	}

	OpusDecoder* Decoder = GetOrCreateDecoder(UUID, SampleRate, NumChannels);
	if (!Decoder)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Failed to create Opus decoder for UUID %s"), *UUID);
		return;
	}

	for (int32 i = 0; i < Lcl_FrameCount; ++i)
	{
		// Frame size
		int32 FrameSize = 0;
		if (Offset + sizeof(int32) > Payload.Num()) break;
		FMemory::Memcpy(&FrameSize, Payload.GetData() + Offset, sizeof(int32));
		Offset += sizeof(int32);

		if (FrameSize <= 0 || Offset + FrameSize > Payload.Num())
		{
			UE_LOG(LogVoiceService, Warning, TEXT("Invalid or incomplete frame %d"), i);
			break;
		}

		// Read frame
		TArray<uint8> EncodedData;
		EncodedData.Append(Payload.GetData() + Offset, FrameSize);
		Offset += FrameSize;

		TArray<float> Decoded;
		if (DecompressAudioData(Decoder, EncodedData, Decoded, SampleRate, NumChannels))
		{
			UE_LOG(LogVoiceService, Log, TEXT("Decoding successful - Decoded samples: %d, Output Float size: %d bytes"), Decoded.Num(), Decoded.Num() * sizeof(float));

			// Launch the broadcast on the game thread
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, DecodedData = MoveTemp(Decoded), SampleRate, NumChannels, UUID]()
			{
				VoiceChatManager->OnAudioDataReceived.Broadcast(DecodedData, SampleRate, NumChannels, UUID);
			}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		}
		else
		{
			UE_LOG(LogVoiceService, Error, TEXT("Decoding failed"));
		}
	}
}

void UVoiceChatServiceSubsystem::SetUDPService(UUDPSubsystem* InUDPService)
{
	UDPSubsystem = InUDPService;
}

void UVoiceChatServiceSubsystem::InitializeOpusEncoder(int32 SampleRate, int32 Channels)
{
	// Additional validation
	if (Channels < 1 || Channels > 2)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Invalid channel count. Must be 1 or 2. Got: %d"), Channels);
		return;
	}

	// Validated sample rates for Opus
	const int ValidSampleRates[] = {8000, 12000, 16000, 24000, 48000};
	bool bValidSampleRate = false;
	for (int ValidRate : ValidSampleRates)
	{
		if (SampleRate == ValidRate)
		{
			bValidSampleRate = true;
			break;
		}
	}

	if (!bValidSampleRate)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Invalid sample rate for Opus. Nearest valid rate is 48000"));
		SampleRate = 48000;
	}

	int Error = OPUS_OK;
	AudioEncoder = opus_encoder_create(
	(SampleRate), 
	(Channels), 
	OPUS_APPLICATION_VOIP, 
	&Error
	);

	if (Error != OPUS_OK)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Opus Encoder creation failed. Error: %d"), Error);
		AudioEncoder = nullptr;
		return;
	}

	// Set encoder parameters
	opus_encoder_ctl(AudioEncoder, OPUS_SET_BITRATE(64000));
	opus_encoder_ctl(AudioEncoder, OPUS_SET_COMPLEXITY(10));
	opus_encoder_ctl(AudioEncoder, OPUS_SET_SIGNAL(OPUS_APPLICATION_VOIP));
}

OpusDecoder* UVoiceChatServiceSubsystem::GetOrCreateDecoder(const FString& UUID, int32 SampleRate, int32 Channels)
{
	FScopeLock Lock(&AudioLock);

	if (OpusDecoder** FoundDecoder = UUIDToDecoderMap.Find(UUID))
	{
		UE_LOG(LogVoiceService, Log, TEXT("Found existing Opus decoder for UUID %s"), *UUID);
		return *FoundDecoder;
	}

	int ErrorCode = 0;

	OpusDecoder* NewDecoder = opus_decoder_create(
		(SampleRate), 
		(Channels), 
		&ErrorCode
	);

	if (ErrorCode != OPUS_OK || !NewDecoder)
	{
		UE_LOG(LogVoiceService, Error, TEXT("Failed to create Opus decoder for UUID %s: %d"), *UUID, ErrorCode);
		return nullptr;
	}

	UUIDToDecoderMap.Add(UUID, NewDecoder);
	UE_LOG(LogVoiceService, Log, TEXT("Created new Opus decoder for UUID %s"), *UUID);
	return NewDecoder;

}

void UVoiceChatServiceSubsystem::CleanupDecoder(const FString& UUID)
{
	FScopeLock Lock(&AudioLock);
    
	if (OpusDecoder** FoundDecoder = UUIDToDecoderMap.Find(UUID))
	{
		opus_decoder_destroy(*FoundDecoder);
		UUIDToDecoderMap.Remove(UUID);
		UE_LOG(LogVoiceService, Log, TEXT("Destroyed Opus decoder for UUID %s"), *UUID);
	}
}

void UVoiceChatServiceSubsystem::CleanupAllDecoders()
{
	FScopeLock Lock(&AudioLock);
    
	for (const auto& Pair : UUIDToDecoderMap)
	{
		if (Pair.Value)
		{
			opus_decoder_destroy(Pair.Value);
		}
	}
	
	UUIDToDecoderMap.Empty();
	UE_LOG(LogVoiceService, Log, TEXT("Destroyed all Opus decoders"));
}

void UVoiceChatServiceSubsystem::CleanupOpus()
{
	if (AudioEncoder)
	{
		opus_encoder_destroy(AudioEncoder);
		AudioEncoder = nullptr;
	}
	UE_LOG(LogVoiceService, Log, TEXT("Destroyed Opus Encoder"));
}
