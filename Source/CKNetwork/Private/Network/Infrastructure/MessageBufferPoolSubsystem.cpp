// Fill out your copyright notice in the Description page of Project Settings.
#include "CKNetwork/Pubilc/Network/Infrastructure//MessageBufferPoolSubsystem.h"

DEFINE_LOG_CATEGORY(BufferPoolLog)

void UMessageBufferPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	DefaultBufferSize = 8192;
	MaxPoolSize = 10000;

	// Initialize Buffer Pool
	InitializeBufferPool(MaxPoolSize, DefaultBufferSize);
}

void UMessageBufferPoolSubsystem::Deinitialize()
{
	Super::Deinitialize();
	for (TArray<uint8>* Buffer: BufferPool)
	{
		delete Buffer;
	}
	BufferPool.Empty();
}

void UMessageBufferPoolSubsystem::PostSubsystemInit()
{
	ISubsystemInitializable::PostSubsystemInit();
}

void UMessageBufferPoolSubsystem::InitializeBufferPool(const int32 InMaxBufferPoolSize, const int32 InBufferSize)
{
	DefaultBufferSize = InBufferSize;
	MaxPoolSize = InMaxBufferPoolSize;

	for (int i = 0; i < MaxPoolSize; i++)
	{
		BufferPool.Add(AllocateBuffer(DefaultBufferSize));
	}

	UE_LOG(BufferPoolLog, Log, TEXT("UMessageBufferPool::Initialize() called. Initialized %d buffers."), MaxPoolSize);
}

TArray<uint8>* UMessageBufferPoolSubsystem::GetBuffer()
{
	{
		FScopeLock Lock(&BufferPoolLock);
		if (BufferPool.Num() > 0)
		{
			TArray<uint8>* Buffer = BufferPool.Pop();
			Buffer->Reset();
			//UE_LOG(BufferPoolLog, Log, TEXT("UMessageBufferPool::GetBuffer() called from pool"));
			return Buffer;
		}
		return nullptr;
	}
}

void UMessageBufferPoolSubsystem::ReleaseBuffer(TArray<uint8>* BufferToRelease)
{
	{
		FScopeLock Lock(&BufferPoolLock);
		if (BufferToRelease)
		{
			if (BufferPool.Num() < MaxPoolSize)
			{
				BufferPool.Add(BufferToRelease);
				//UE_LOG(BufferPoolLog, Log, TEXT("UMessageBufferPool::Buffer Released back to pool."))
			}
			else
			{
				delete BufferToRelease;
				//UE_LOG(BufferPoolLog, Log, TEXT("UMessageBufferPool::Pool is full. Buffer was destroyed."))
			}
		}
	}
}

TArray<uint8>* UMessageBufferPoolSubsystem::AllocateBuffer(const int32 BufferSize)
{
	TArray<uint8>* NewBuffer = new TArray<uint8>();
	NewBuffer->SetNumUninitialized(BufferSize);
	return NewBuffer;
}





