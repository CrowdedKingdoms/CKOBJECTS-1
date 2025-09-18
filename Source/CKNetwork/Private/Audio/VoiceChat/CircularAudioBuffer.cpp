// CircularAudioBuffer.cpp
#include "CKNetwork/Pubilc/Audio/VoiceChat/CircularAudioBuffer.h"

FCircularAudioBuffer::FCircularAudioBuffer(uint32 InSampleRate, uint32 InNumChannels, float InBufferDuration)
{
    // Calculate buffer capacity based on sample rate, channels and duration
    BufferCapacity = static_cast<int32>(InSampleRate * InNumChannels * InBufferDuration);
    
    // Initialize the buffer
    Buffer.SetNumZeroed(BufferCapacity);
    
    // Initialize indices
    ReadIndex = 0;
    WriteIndex = 0;
    SamplesAvailable = 0;
    
    // Initialize state
    bIsEmpty = true;
    bIsFull = false;
}

FCircularAudioBuffer::~FCircularAudioBuffer()
{
    // Clean up resources
    FScopeLock Lock(&BufferLock);
    Buffer.Empty();
}

bool FCircularAudioBuffer::TryEnqueue(const float* InData, int32 InNumSamples)
{
    if (InData == nullptr || InNumSamples <= 0)
    {
        return false;
    }
    
    FScopeLock Lock(&BufferLock);
    
    // Handle case where input exceeds buffer capacity
    if (InNumSamples > BufferCapacity)
    {
        // Only use the most recent samples that fit in the buffer
        InData += (InNumSamples - BufferCapacity);
        InNumSamples = BufferCapacity;
    }
    
    // Add data sample by sample, handling wrap-around
    for (int32 i = 0; i < InNumSamples; ++i)
    {
        Buffer[WriteIndex] = InData[i];
        
        // Update write index with wrap-around
        WriteIndex = (WriteIndex + 1) % BufferCapacity;
        
        // If buffer was full, move read index too (overwrite oldest data)
        if (bIsFull)
        {
            ReadIndex = (ReadIndex + 1) % BufferCapacity;
        }
        else
        {
            // Increment available samples count
            SamplesAvailable++;
            if (SamplesAvailable >= BufferCapacity)
            {
                bIsFull = true;
                SamplesAvailable = BufferCapacity;
            }
        }
        
        bIsEmpty = false;
    }
    
    return true;
}

bool FCircularAudioBuffer::TryEnqueue(const TArray<float>& InData)
{
    if (InData.Num() <= 0)
    {
        return false;
    }
    
    return TryEnqueue(InData.GetData(), InData.Num());
}

bool FCircularAudioBuffer::TryDequeue(TArray<float>& OutData, int32 InNumSamples)
{
    FScopeLock Lock(&BufferLock);
    
    // Can't dequeue if buffer is empty or requested samples exceed available data
    if (bIsEmpty || InNumSamples <= 0 || InNumSamples > SamplesAvailable)
    {
        return false;
    }
    
    // Resize output array
    OutData.SetNumUninitialized(InNumSamples);
    
    // Extract data sample by sample, handling wrap-around
    for (int32 i = 0; i < InNumSamples; ++i)
    {
        OutData[i] = Buffer[ReadIndex];
        
        // Update read index with wrap-around
        ReadIndex = (ReadIndex + 1) % BufferCapacity;
    }
    
    // Update state
    SamplesAvailable -= InNumSamples;
    bIsFull = false;
    bIsEmpty = (SamplesAvailable == 0);
    
    return true;
}

int32 FCircularAudioBuffer::GetAvailableDataSize() const
{
    FScopeLock Lock(&const_cast<FCriticalSection&>(BufferLock));
    return SamplesAvailable;
}

void FCircularAudioBuffer::Reset()
{
    FScopeLock Lock(&BufferLock);
    
    // Reset indices and state
    ReadIndex = 0;
    WriteIndex = 0;
    SamplesAvailable = 0;
    bIsEmpty = true;
    bIsFull = false;
    
    // Clear buffer data
    FMemory::Memzero(Buffer.GetData(), Buffer.Num() * sizeof(float));
}