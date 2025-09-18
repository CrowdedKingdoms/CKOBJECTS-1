// CircularAudioBuffer.h
#pragma once

#include "CoreMinimal.h"

/**
 * A thread-safe circular buffer implementation for storing audio data
 * Provides fixed-capacity FIFO operations with automatic overwrite of oldest data
 */
class FCircularAudioBuffer
{
public:
    /**
     * Constructor for circular audio buffer
     * 
     * @param InSampleRate The sample rate of the audio data (samples per second)
     * @param InNumChannels Number of audio channels (1 for mono, 2 for stereo)
     * @param InBufferDuration Duration of buffer in seconds
     */
    FCircularAudioBuffer(uint32 InSampleRate = 48000, uint32 InNumChannels = 1, float InBufferDuration = 2.0f);
    
    /**
     * Destructor - cleans up buffer memory
     */
    ~FCircularAudioBuffer();
    
    /**
     * Attempts to add audio data to the buffer
     * If buffer is full, oldest data will be overwritten
     * 
     * @param InData Pointer to audio data to enqueue
     * @param InNumSamples Number of samples to enqueue
     * @return True if data was successfully enqueued
     */
    bool TryEnqueue(const float* InData, int32 InNumSamples);
    
    /**
     * Attempts to add audio data to the buffer from a TArray
     * 
     * @param InData Array of audio data to enqueue
     * @return True if data was successfully enqueued
     */
    bool TryEnqueue(const TArray<float>& InData);
    
    /**
     * Attempts to retrieve audio data from the buffer
     * 
     * @param OutData Array where dequeued data will be stored
     * @param InNumSamples Number of samples to dequeue
     * @return True if data was successfully dequeued
     */
    bool TryDequeue(TArray<float>& OutData, int32 InNumSamples);
    
    /**
     * Gets the amount of data available to read from the buffer
     * 
     * @return Number of samples available
     */
    int32 GetAvailableDataSize() const;
    
    /**
     * Clears all data from the buffer
     */
    void Reset();
    
    /**
     * Gets the total capacity of the buffer in samples
     * 
     * @return Buffer capacity
     */
    int32 GetCapacity() const { return BufferCapacity; }

private:
    /** Buffer storage */
    TArray<float> Buffer;
    
    /** Critical section for thread safety */
    FCriticalSection BufferLock;
    
    /** Current write position */
    int32 WriteIndex;
    
    /** Current read position */
    int32 ReadIndex;
    
    /** Number of samples currently in the buffer */
    int32 SamplesAvailable;
    
    /** Total buffer capacity in samples */
    int32 BufferCapacity;
    
    /** Is buffer empty */
    bool bIsEmpty;
    
    /** Is buffer full */
    bool bIsFull;
};