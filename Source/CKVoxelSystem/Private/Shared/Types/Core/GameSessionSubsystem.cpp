// Fill out your copyright notice in the Description page of Project Settings.
#include "Shared/Types/Core/GameSessionSubsystem.h"

void UGameSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UGameSessionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}


bool UGameSessionSubsystem::EnqueueMessageToSend(const TArray<uint8>& Message)
{
	SendCounter.Increment();
	return SendQueue.Enqueue(Message);
}

void UGameSessionSubsystem::DequeueMessageToSend(TArray<uint8>& OutMessage)
{
	SendCounter.Decrement();
	SendQueue.Dequeue(OutMessage);
}

void UGameSessionSubsystem::EnqueueMessageToReceive(const TArray<uint8>& Message)
{
	FScopeLock Lock(&ReceiveQueueMutex);
	ReceiveQueue.Enqueue(Message);
	ReceiveCounter.Increment();
}

bool UGameSessionSubsystem::DequeueMessageToReceive(TArray<uint8>& OutMessage)
{
	FScopeLock Lock(&ReceiveQueueMutex);

	if (ReceiveQueue.IsEmpty())
	{
		return false;
	}
	
	ReceiveCounter.Decrement();
	ReceiveQueue.Dequeue(OutMessage);
	return true;
}

void UGameSessionSubsystem::PrintCounterValues() const
{
	UE_LOG(LogTemp, Log, TEXT("Send Queue: %d"), SendCounter.GetValue());
	UE_LOG(LogTemp, Log, TEXT("Receive Queue: %d"), ReceiveCounter.GetValue());
}

void UGameSessionSubsystem::ClearSessionData()
{
	MapID = 0;
	GameToken.Empty();
	PlayerUUID.Empty();
	UserID = 0;
	GameTokenID = 0;
	CurrentPlayerChunkCoordinates = FInt64Vector(0, 0, 0);
	
	UE_LOG(LogTemp, Log, TEXT("GameSessionSubsystem: Session data cleared"));
}

bool UGameSessionSubsystem::HasPendingIncomingMessages() const
{
	return !ReceiveQueue.IsEmpty();
}

bool UGameSessionSubsystem::HasPendingOutgoingMessages() const
{
	return !SendQueue.IsEmpty();
}




