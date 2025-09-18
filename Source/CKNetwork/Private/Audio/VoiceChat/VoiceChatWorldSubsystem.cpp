﻿// Fill out your copyright notice in the Description page of Project Settings.
#include "CKNetwork/Pubilc/Audio/VoiceChat/VoiceChatWorldSubsystem.h"

#include "CKNetwork/Pubilc/Audio/Platform/WindowsAudioDeviceMonitor.h"
#include "CKNetwork/Pubilc/Audio/VoiceChat/CircularAudioBuffer.h"
#include "CKNetwork/Pubilc/Audio/VoiceChat/Structures/FAudioTrack.h"
#include "Engine/Engine.h"
#include "Generators/AudioGenerator.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"

DEFINE_LOG_CATEGORY(LogVoiceChat);

void UVoiceChatWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Initialize default values
	TargetNumChannels = 1;
	TargetSampleRate = 48000;
	AUDIO_CHUNK_SIZE = 960;

	// Create a capture buffer with 2-second capacity
	CaptureBuffer = new FCircularAudioBuffer(96000, 1, 2.0f);

	AudioResampler = nullptr;
	LastDeviceSampleRate = 0;
}

void UVoiceChatWorldSubsystem::Deinitialize()
{
	// Clean up resources
	StopVoiceChat();

	// Delete buffers
	if (CaptureBuffer)
	{
		delete CaptureBuffer;
		CaptureBuffer = nullptr;
	}

	// Clean up player streams
	for (auto& StreamPair : SoundStreamMap)
	{
		FAudioTrack& AudioTrack = StreamPair.Value;
		if (AudioTrack.PlaybackBuffer)
		{
			delete AudioTrack.PlaybackBuffer;
			AudioTrack.PlaybackBuffer = nullptr;
		}
	}

	if (AudioResampler)
	{
		src_delete(AudioResampler);
		AudioResampler = nullptr;
	}

	Super::Deinitialize();
}

void UVoiceChatWorldSubsystem::PostSubsystemInit()
{
	ISubsystemInitializable::PostSubsystemInit();
	// Initialize audio capture system
	InitializeAudioCapture();

	// Component Initialization
	AudioDeviceMonitor = NewObject<AWindowsAudioDeviceMonitor>(GetWorld());


	AudioDeviceMonitor->OnAudioDeviceConnected.AddDynamic(this, &UVoiceChatWorldSubsystem::HandleAudioDeviceConnection);
	AudioDeviceMonitor->OnAudioDeviceDisconnected.AddDynamic(
		this, &UVoiceChatWorldSubsystem::HandleAudioDeviceDisconnection);

	RootActor = GetWorld()->GetFirstPlayerController()->GetPawn();
}

TStatId UVoiceChatWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVoiceChatWorldSubsystem, STATGROUP_Tickables);
}

void UVoiceChatWorldSubsystem::Tick(const float DeltaTime)
{
}

void UVoiceChatWorldSubsystem::TickPlayback()
{
	if (bIsTicking)
	{
		return;
	}

	bIsTicking = true;
	
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
	{
		double LastTime = FPlatformTime::Seconds();
		double LocalAccumulatedTime = 0.0;
		constexpr double LocalTickInterval = 0.02; // ~60 FPS equivalent, adjust as needed
		
		while (bIsTicking)
		{
			bool bProcessedWork = false;
			
			const double CurrentTime = FPlatformTime::Seconds();
			const double DeltaTime = CurrentTime - LastTime;
			LastTime = CurrentTime;
			
			LocalAccumulatedTime += DeltaTime;
			
			if (LocalAccumulatedTime >= LocalTickInterval)
			{
				// Process audio playback
				if (bIsProcessingIncomingAudio)
				{
					ManagePlaybackBuffer();
					bProcessedWork = true;
				}
				
				// Check for timed-out streams
				CheckStreamTimeouts();
				bProcessedWork = true;
				
				LocalAccumulatedTime -= LocalTickInterval;
			}
			
			// Only sleep if no work was processed
			if (!bProcessedWork)
			{
				FPlatformProcess::Sleep(0.001f); // 1ms
			}
		}
	});
}

void UVoiceChatWorldSubsystem::StopTicking()
{
	bIsTicking = false;
}

void UVoiceChatWorldSubsystem::InitializeAudioCapture()
{
	// Configure audio capture device
	// Create audio capture device
	AudioCaptureDevice = NewObject<UAudioCapture>(this);

	if (AudioCaptureDevice)
	{
		AudioCaptureDevice->AddGeneratorDelegate(FOnAudioGenerate([this](const float* AudioData, int32 NumSamples)
		{
			HandleAudioGeneration(AudioData, NumSamples);
		}));
	}
}

void UVoiceChatWorldSubsystem::StartVoiceChat()
{
	UE_LOG(LogVoiceChat, Log, TEXT("Starting voice chat"));
	InitializeAudioCapture();
	StartVoiceCapture();
	bVoiceChatIsRunning = true;
}

void UVoiceChatWorldSubsystem::StopVoiceChat()
{
	UE_LOG(LogVoiceChat, Log, TEXT("Stopping voice chat"));
	StopVoiceCapture();
	bVoiceChatIsRunning = false;
}

void UVoiceChatWorldSubsystem::PlayVoiceChat()
{
	UE_LOG(LogVoiceChat, Log, TEXT("Playing voice chat"));

	if (!OnAudioDataReceived.IsBound())
	{
		OnAudioDataReceived.AddDynamic(this, &UVoiceChatWorldSubsystem::HandleIncomingAudio);
	}

	bIsProcessingIncomingAudio = true;

	TickPlayback();
}

void UVoiceChatWorldSubsystem::MuteVoiceChat()
{
	UE_LOG(LogVoiceChat, Log, TEXT("Muting voice chat"));
	OnAudioDataReceived.Clear();
	bIsProcessingIncomingAudio = false;

	StopTicking();
}

void UVoiceChatWorldSubsystem::StartVoiceCapture()
{
	if (!bIsCapturing && AudioCaptureDevice)
	{
		// Reset buffers
		CaptureBuffer->Reset();

		// Start the audio capture
		bool bSuccess = AudioCaptureDevice->OpenDefaultAudioStream();

		if (!bSuccess)
		{
			UE_LOG(LogVoiceChat, Error, TEXT("Failed to open default audio stream"));
			return;
		}

		if (!AudioCaptureDevice->IsCapturingAudio())
		{
			AudioCaptureDevice->StartCapturingAudio();
			bIsCapturing = true;
			bIsProcessing = true;

			// Start the processing thread
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				ProcessAudioDataThread();
			}, LowLevelTasks::ETaskPriority::BackgroundLow);
		}
		UE_LOG(LogVoiceChat, Log, TEXT("Voice capture started"));
	}
}

void UVoiceChatWorldSubsystem::StopVoiceCapture()
{
	if (bIsCapturing && AudioCaptureDevice && AudioCaptureDevice->IsCapturingAudio())
	{
		// Stop the audio capture
		AudioCaptureDevice->StopCapturingAudio();
		bIsCapturing = false;
		bIsProcessing = false;
		AudioCaptureDevice = nullptr;
		UE_LOG(LogVoiceChat, Log, TEXT("Voice capture stopped"));
	}
}

void UVoiceChatWorldSubsystem::HandleAudioGeneration(const float* AudioData, int32 NumSamples)
{
	
	if (!bIsCapturing)
	{
		return;
	}
	
	// Get current device sample rate
	const int32 DeviceSampleRate = AudioCaptureDevice->GetSampleRate();
	const int32 DeviceChannels = AudioCaptureDevice->GetNumChannels();

	// Calculate time length of this audio chunk in seconds
	const float ChunkTimeSeconds = static_cast<float>(NumSamples) / (DeviceSampleRate * DeviceChannels);

	// Calculate the equivalent number of samples at our target rate (48kHz)
	const int32 TargetNumSamples = FMath::RoundToInt(ChunkTimeSeconds * TargetSampleRate);

	// First, convert to mono if necessary
	const float* AudioToProcess = AudioData;
	const int32 MonoSampleCount = NumSamples / DeviceChannels;

	if (DeviceChannels > 1)
	{
		TArray<float> MonoAudio;
		MonoAudio.Reserve(MonoSampleCount);

		for (int32 i = 0; i < NumSamples; i += DeviceChannels)
		{
			float MonoSample = 0.0f;
			for (int32 Channel = 0; Channel < DeviceChannels; ++Channel)
			{
				MonoSample += AudioData[i + Channel];
			}
			MonoSample /= DeviceChannels; // Average across all channels
			MonoAudio.Add(MonoSample);
		}

		AudioToProcess = MonoAudio.GetData();
	}

	// If the sample rate doesn't match our target, resample
	if (DeviceSampleRate != TargetSampleRate)
	{
		// Check if we need to create or update the resampler
		if (!AudioResampler || LastDeviceSampleRate != DeviceSampleRate)
		{
			// Create or recreate resampler with the right conversion ratio
			if (AudioResampler)
			{
				src_delete(AudioResampler);
			}

			int Error;
			AudioResampler = src_new(SRC_SINC_BEST_QUALITY, 1, &Error);
			LastDeviceSampleRate = DeviceSampleRate;

			if (Error != 0)
			{
				UE_LOG(LogVoiceChat, Error, TEXT("Failed to create resampler: %s"),
				       UTF8_TO_TCHAR(src_strerror(Error)));
				return;
			}
		}

		// Prepare resampling
		const double ResampleRatio = static_cast<double>(TargetSampleRate) / DeviceSampleRate;

		TArray<float> ResampledAudio;
		ResampledAudio.SetNumUninitialized(TargetNumSamples + 32); // Add padding

		SRC_DATA SrcData;
		SrcData.data_in = const_cast<float*>(AudioToProcess);
		SrcData.data_out = ResampledAudio.GetData();
		SrcData.input_frames = MonoSampleCount;
		SrcData.output_frames = TargetNumSamples + 32;
		SrcData.src_ratio = ResampleRatio;
		SrcData.end_of_input = 0; // We're preserving state between calls

		// Process with resampler state preservation for smooth transitions
		int Error = src_process(AudioResampler, &SrcData);

		if (Error != 0)
		{
			UE_LOG(LogVoiceChat, Error, TEXT("Resampling error: %s"),
			       UTF8_TO_TCHAR(src_strerror(Error)));
			return;
		}

		// Resize to actual output size
		ResampledAudio.SetNum(SrcData.output_frames_gen);

		// Add resampled data to buffer
		if (CaptureBuffer && ResampledAudio.Num() > 0)
		{
			CaptureBuffer->TryEnqueue(ResampledAudio.GetData(), ResampledAudio.Num());
		}
	}
	else
	{
		// Sample rate already matches target, add directly to buffer
		if (CaptureBuffer)
		{
			CaptureBuffer->TryEnqueue(AudioToProcess, MonoSampleCount);
		}
	}
}

void UVoiceChatWorldSubsystem::ProcessAudioDataThread() const
{
	TArray<float> AudioChunk;

	while (bIsProcessing)
	{
		bool bProcessedAudio = false;
		
		// Lock for thread safety
		{
			// Check if we have enough data to process
			if (CaptureBuffer && CaptureBuffer->GetAvailableDataSize() >= AUDIO_CHUNK_SIZE)
			{
				// Dequeue audio data for processing
				if (CaptureBuffer->TryDequeue(AudioChunk, AUDIO_CHUNK_SIZE))
				{
					bProcessedAudio = true;
					
					// Broadcast audio data for networking or further processing
					AsyncTask(ENamedThreads::GameThread, [this, AudioChunk]()
					{
						if (OnAudioDataGenerated.IsBound())
						{
							OnAudioDataGenerated.Broadcast(AudioChunk, TargetSampleRate, TargetNumChannels);
						}
					});
				}
			}
		}
		
		// Only sleep if no audio was processed
		if (!bProcessedAudio)
		{
			FPlatformProcess::Sleep(0.01f);
		}
	}
}

void UVoiceChatWorldSubsystem::ManagePlaybackBuffer()
{
	// Lock for thread safety
	FScopeLock Lock(&AudioCriticalSection);

	// Process each stream
	StreamsToProcess.Reset();

	// Build list of streams to process
	for (auto& StreamPair : SoundStreamMap)
	{
		StreamsToProcess.Add(TPair<FString, FAudioTrack*>(StreamPair.Key, &StreamPair.Value));
	}

	// Process each stream
	for (auto& StreamPair : StreamsToProcess)
	{
		ProcessPlayerStream(StreamPair.Value);
	}
}

void UVoiceChatWorldSubsystem::HandleIncomingAudio(const TArray<float>& IncomingAudioData, int32 SampleRate,
                                                   int32 NumChannels, FString PlayerID)
{
	// Lock for thread safety
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, IncomingAudioData, SampleRate, NumChannels, PlayerID]()
	{
		FScopeLock Lock(&AudioCriticalSection);
		UE_LOG(LogVoiceChat, Log, TEXT("Incoming audio data received for PlayerID: %s"), *PlayerID);

		// Check if we have a stream for this player, create if not
		if (!DoesPlayerStreamExist(PlayerID))
		{
			UE_LOG(LogVoiceChat, Log, TEXT("No stream found for PlayerID: %s, creating a new one."), *PlayerID);

			AsyncTask(ENamedThreads::GameThread, [this, PlayerID, SampleRate]()
			{
				AddPlayerStream(PlayerID, SampleRate);
			});
			return; // Return early if we don't have a stream for this player
		}

		// Get the player's stream
		FAudioTrack* PlayerStream = GetPlayerStream(PlayerID);
		if (PlayerStream && PlayerStream->PlaybackBuffer)
		{
			// Add audio data to the player's buffer
			const float* AudioData = IncomingAudioData.GetData();

			if (!PlayerStream->PlaybackBuffer->TryEnqueue(AudioData, IncomingAudioData.Num()))
			{
				UE_LOG(LogVoiceChat, Warning, TEXT("Failed to enqueue audio data for PlayerID: %s"), *PlayerID);
			}

			// Update last activity timestamp
			LastUpdateTimes.Add(PlayerID, FPlatformTime::Seconds());

			//UE_LOG(LogVoiceChat, Log, TEXT("Updated last activity timestamp for PlayerID: %s"), *PlayerID);
		}
		else
		{
			UE_LOG(LogVoiceChat, Warning,
			       TEXT("Player stream is invalid or playback buffer not initialized for PlayerID: %s"), *PlayerID);
		}
	}, LowLevelTasks::ETaskPriority::BackgroundHigh);
}

bool UVoiceChatWorldSubsystem::DoesPlayerStreamExist(const FString& PlayerID) const
{
	return SoundStreamMap.Contains(PlayerID);
}

FAudioTrack* UVoiceChatWorldSubsystem::GetPlayerStream(const FString& PlayerID)
{
	if (DoesPlayerStreamExist(PlayerID))
	{
		return &SoundStreamMap[PlayerID];
	}
	return nullptr;
}

void UVoiceChatWorldSubsystem::AddPlayerStream(const FString& PlayerID, uint32 SampleRate)
{
	// Create a new audio track for this player
	FAudioTrack NewTrack;

	// Create procedural sound wave
	NewTrack.SoundWave = NewObject<USoundWaveProcedural>();
	if (NewTrack.SoundWave)
	{
		NewTrack.SoundWave->SetSampleRate(SampleRate);
		NewTrack.SoundWave->NumChannels = 1;
		NewTrack.SoundWave->Duration = INDEFINITELY_LOOPING_DURATION;
		NewTrack.SoundWave->SoundGroup = SOUNDGROUP_Voice;
		NewTrack.SoundWave->bLooping = false;
	}

	// Create audio component for playback
	NewTrack.AudioComponent = NewObject<UAudioComponent>(RootActor);
	if (NewTrack.AudioComponent)
	{
		NewTrack.AudioComponent->SetSound(NewTrack.SoundWave);
		NewTrack.AudioComponent->bAutoActivate = true;
		NewTrack.AudioComponent->RegisterComponent();
	}

	// Create buffer for playback (2 second capacity)
	NewTrack.PlaybackBuffer = new FCircularAudioBuffer(SampleRate, 1, 2.0f);

	// Add to map
	SoundStreamMap.Add(PlayerID, NewTrack);
	LastUpdateTimes.Add(PlayerID, FPlatformTime::Seconds());

	UE_LOG(LogVoiceChat, Log, TEXT("Added player %s to voice chat"), *PlayerID);
}

void UVoiceChatWorldSubsystem::RemovePlayerStream(const FString& PlayerID)
{
	// Get the player's stream
	FAudioTrack* PlayerStream = GetPlayerStream(PlayerID);
	if (PlayerStream)
	{
		// For Voice Chat Service
		OnStreamTimeout.Broadcast(PlayerID);

		// Clean up resources
		if (PlayerStream->AudioComponent)
		{
			PlayerStream->AudioComponent->Stop();
			PlayerStream->AudioComponent->DestroyComponent();
		}

		if (PlayerStream->PlaybackBuffer)
		{
			delete PlayerStream->PlaybackBuffer;
		}

		// Remove from maps
		SoundStreamMap.Remove(PlayerID);
		LastUpdateTimes.Remove(PlayerID);

		UE_LOG(LogVoiceChat, Log, TEXT("Removed player %s from voice chat"), *PlayerID);
	}
}

void UVoiceChatWorldSubsystem::ProcessPlayerStream(FAudioTrack* AudioTrack)
{
	if (!AudioTrack || !AudioTrack->PlaybackBuffer || !AudioTrack->SoundWave)
	{
		return;
	}

	// Number of samples to read for this update
	constexpr int32 MaxSamplesToRead = 960; // Adjust this for your needs
	constexpr int32 MinSamplesToRead = 512; // Adjust this for your needs

	const int32 AvailableSamples = AudioTrack->PlaybackBuffer->GetAvailableDataSize();

	// Process if we have enough data
	if (AvailableSamples >= MinSamplesToRead)
	{
		const int32 SamplesToRead = FMath::Min(AvailableSamples, MaxSamplesToRead);

		TArray<float> AudioChunk;

		if (AudioTrack->PlaybackBuffer->TryDequeue(AudioChunk, SamplesToRead))
		{
			// Convert float [-1.0f, 1.0f] to 16-bit PCM
			TArray<int16> PCMInt16Data;
			PCMInt16Data.SetNumUninitialized(AudioChunk.Num());

			for (int32 i = 0; i < AudioChunk.Num(); ++i)
			{
				const float ClampedSample = FMath::Clamp(AudioChunk[i], -1.0f, 1.0f);
				PCMInt16Data[i] = static_cast<int16>(ClampedSample * 32767.0f);
			}

			// Convert to uint8 buffer for QueueAudio
			TArray<uint8> PCMData;
			PCMData.SetNumUninitialized(PCMInt16Data.Num() * sizeof(int16));
			FMemory::Memcpy(PCMData.GetData(), PCMInt16Data.GetData(), PCMData.Num());

			// Queue to sound wave
			AudioTrack->SoundWave->QueueAudio(PCMData.GetData(), PCMData.Num());

			// Start playback if not playing
			if (AudioTrack->AudioComponent && !AudioTrack->AudioComponent->IsPlaying())
			{
				AudioTrack->AudioComponent->Play();
			}
		}
	}
}

void UVoiceChatWorldSubsystem::CheckStreamTimeouts()
{
	// Current time
	const double CurrentTime = FPlatformTime::Seconds();
	TArray<FString> StreamsToRemove;

	// Check each stream for timeout
	for (auto& TimePair : LastUpdateTimes)
	{
		if (CurrentTime - TimePair.Value > StreamTimeoutThreshold)
		{
			StreamsToRemove.Add(TimePair.Key);
		}
	}

	// Remove timed-out streams
	for (const FString& PlayerID : StreamsToRemove)
	{
		RemovePlayerStream(PlayerID);
	}
}

void UVoiceChatWorldSubsystem::HandleAudioDeviceDisconnection(const FString& DeviceID)
{
	UE_LOG(LogVoiceChat, Log, TEXT("Audio device disconnected: %s"), *DeviceID);
	// Reset the voice chat state completely
	bIsCapturing = false;
	bIsProcessing = false;

	// Make sure the audio device is properly released
	if (AudioCaptureDevice)
	{
		if (AudioCaptureDevice->IsCapturingAudio())
		{
			AudioCaptureDevice->StopCapturingAudio();
		}
		AudioCaptureDevice = nullptr;
	}
}

void UVoiceChatWorldSubsystem::HandleAudioDeviceConnection(const FString& DeviceID)
{
	UE_LOG(LogVoiceChat, Log, TEXT("Audio device connected: %s"), *DeviceID);

	// automatic restart:
	if (bVoiceChatIsRunning)
	{
		// Make sure we're starting from a clean state
		StopVoiceChat();
		// Wait a short time to ensure the device is ready
		FTimerHandle RestartTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			RestartTimerHandle,
			[this]() { StartVoiceChat(); },
			0.5f, // Half-second delay
			false
		);
	}
}
