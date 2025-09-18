#pragma once
#include "CoreMinimal.h"
#include "CKNetwork/Pubilc/Audio/VoiceChat/CircularAudioBuffer.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "UObject/WeakObjectPtr.h"
#include "FAudioTrack.generated.h"

USTRUCT()
struct FAudioTrack
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	/** The procedural sound wave used for playback */
	UPROPERTY()
	TObjectPtr<USoundWaveProcedural> SoundWave;
    
	/** The audio component that plays the sound wave */
	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComponent;
    
	/** Buffer for audio playback */
	FCircularAudioBuffer* PlaybackBuffer;
};