// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioCapture.h"
#include "Audio/VoiceChat/CircularAudioBuffer.h"
#include "Audio/Platform/WindowsAudioDeviceMonitor.h"
#include "samplerate.h"
#include "Audio/VoiceChat/Structures/FAudioTrack.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "VoiceChatWorldSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVoiceChat, Log, All);

// Event Delegates for Audio Generation and Reception
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAudioDataGenerated, const TArray<float>&, AudioData, int32, SampleRate, int32, NumChannels);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAudioDataReceived, const TArray<float>&, AudioData, int32, SampleRate, int32, NumChannels, FString, PlayerID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreamTimeout, const FString&, UUID);

/**
 * Manages voice chat functionality including audio capture, processing, and playback
 * for multiple users in a networked environment
 */
UCLASS(Blueprintable, BlueprintType)
class  UVoiceChatWorldSubsystem : public UTickableWorldSubsystem, public ISubsystemInitializable
{
    GENERATED_BODY()
    
public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    virtual void PostSubsystemInit() override;

    virtual TStatId GetStatId() const override;
    
    /** Fired when local audio is generated and ready for transmission */
    UPROPERTY(BlueprintAssignable, Category="Voice Chat")
    FOnAudioDataGenerated OnAudioDataGenerated;
    
    /** Fired when audio is received from a remote player */
    UPROPERTY(BlueprintAssignable, Category="Voice Chat")
    FOnAudioDataReceived OnAudioDataReceived;


    UPROPERTY(BlueprintAssignable, Category="Voice Chat")
    FOnStreamTimeout OnStreamTimeout;
    
    /** Begin capturing and processing local audio */
    UFUNCTION(BlueprintCallable, Category="Voice Chat")
    void StartVoiceChat();
    
    /** Stop capturing and processing local audio */
    UFUNCTION(BlueprintCallable, Category="Voice Chat")
    void StopVoiceChat();
    
    /** Begin playing received audio */
    UFUNCTION(BlueprintCallable, Category="Voice Chat")
    void PlayVoiceChat();
    
    /** Mute all received audio */
    UFUNCTION(BlueprintCallable, Category="Voice Chat")
    void MuteVoiceChat();
    
    /** Set the timeout threshold for inactive streams */
    UFUNCTION(BlueprintCallable, Category="Voice Chat")
    void SetStreamTimeoutThreshold(const float InSeconds) { StreamTimeoutThreshold = InSeconds; }
    
protected:
    
    /** Called every frame */
    virtual void Tick(float DeltaTime) override;

    void TickPlayback();
    void StopTicking();

    FThreadSafeBool bIsTicking = false;
    
    /** The device used to capture audio from the microphone */
    UPROPERTY()
    UAudioCapture* AudioCaptureDevice;
    
    /** Initialize the audio capture device with appropriate settings */
    void InitializeAudioCapture();
    
    /** Start capturing audio from the microphone */
    void StartVoiceCapture();
    
    /** Stop capturing audio from the microphone */
    void StopVoiceCapture();
    
    /** Process captured audio data from the microphone */
    void HandleAudioGeneration(const float* AudioData, int32 NumSamples);
    
    /** Thread function for processing audio data */
    void ProcessAudioDataThread() const;
    
    /** Manages the buffer for audio playback */
    void ManagePlaybackBuffer();
    
    /** Handles incoming audio data from a remote player */
    UFUNCTION()
    void HandleIncomingAudio(const TArray<float>& IncomingAudioData, int32 SampleRate, int32 NumChannels, FString PlayerID);
    
    /** Time in seconds after which an inactive stream is considered timed out */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Voice Chat")
    float StreamTimeoutThreshold = 2.0f;
    
private:
    /** Buffer for captured audio data */
    FCircularAudioBuffer* CaptureBuffer;
    
    /** Critical section for thread synchronization */
    FCriticalSection AudioCriticalSection;
    
    /** Number of audio channels for capture */
    UPROPERTY()
    int32 TargetNumChannels;
    
    /** Sample rate for audio capture (Hz) */
    UPROPERTY()
    int32 TargetSampleRate;
    
    /** Flag indicating if audio is currently being captured */
    std::atomic<bool> bIsCapturing;
    
    /** Flag indicating if audio is currently being processed */
    std::atomic<bool> bIsProcessing;
    
    /** Flag indicating if incoming audio is being processed */
    std::atomic<bool> bIsProcessingIncomingAudio;
    
    /** Size of audio chunks for processing */
    UPROPERTY()
    int32 AUDIO_CHUNK_SIZE;
    
    /** Map of player IDs to their audio tracks */
    UPROPERTY()
    TMap<FString, FAudioTrack> SoundStreamMap;
    
    /** Tracks to process in parallel */
    TArray<TPair<FString, FAudioTrack*>> StreamsToProcess;
    
    /** Last update time for each player stream */
    TMap<FString, double> LastUpdateTimes;

    /** LeftoverFrames */
    TArray<float> PendingAudioSamples;
    
    /** Check if a player stream already exists */
    bool DoesPlayerStreamExist(const FString& PlayerID) const;
    
    /** Get a player's audio stream */
    FAudioTrack* GetPlayerStream(const FString& PlayerID);
    
    /** Add a new player to the stream map */
    void AddPlayerStream(const FString& PlayerID, uint32 SampleRate);
    
    /** Remove a player from the stream map */
    void RemovePlayerStream(const FString& PlayerID);
    
    /** Process audio for a specific player's stream */
    void ProcessPlayerStream(FAudioTrack* AudioTrack);

    /**
    * Checks for voice chat streams that have timed out and removes them
    * A stream times out when it hasn't received audio data for longer than StreamTimeoutThreshold
    */
    void CheckStreamTimeouts();

    /** Reference for Audio Device Monitor**/
    UPROPERTY()
    AWindowsAudioDeviceMonitor* AudioDeviceMonitor;

    /** Delegate Handler for When an audio device disconnects **/
    UFUNCTION()
    void HandleAudioDeviceDisconnection(const FString& DeviceID);

    /** Delegate Handler for when an audio device is reconnected **/
    UFUNCTION()
    void HandleAudioDeviceConnection(const FString& DeviceID);

    /** Bool for knowing if VC was running before disconnection or not **/
    UPROPERTY()
    bool bVoiceChatIsRunning;

    SRC_STATE* AudioResampler = nullptr;

    UPROPERTY()
    int32 LastDeviceSampleRate = 0;

    UPROPERTY()
    float TickInterval = 0.02f;

    UPROPERTY()
    float AccumulatedTime = 0.0f;

    UPROPERTY()
    AActor* RootActor;
};