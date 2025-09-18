#include "CKNetwork/Pubilc/Network/Infrastructure/WorkerThreadsSubsystem.h"
#include "CKTypes/Public/Shared/Types/Core/GameSessionSubsystem.h"
#include "CKNetwork/Pubilc/Network/Infrastructure/MessageBufferPoolSubsystem.h"
#include "CKNetwork/Pubilc/Network/Infrastructure/NetworkMessageParser.h"
#include "CKNetwork/Pubilc/Network/Services/Core/UDPSubsystem.h"
#include "Async/Async.h"
#include "Public/Threads/FWorker.h"


void UWorkerThreadsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UWorkerThreadsSubsystem::Deinitialize()
{
	bShouldRun = false;
	bProcessMainThread = false;
	StopAllProcesses();
	Super::Deinitialize();
}

void UWorkerThreadsSubsystem::PostSubsystemInit()
{
	BufferPoolSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMessageBufferPoolSubsystem>();
	NetworkMessageParser = GetWorld()->GetGameInstance()->GetSubsystem<UNetworkMessageParser>();
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();

	if (!BufferPoolSubsystem || !NetworkMessageParser || !GameSessionSubsystem || !UDPSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("WorkerThreadSubsystem::Some or one of the subsystem is invalid."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Worker Thread Subsystem Initialized."));
	InitializeWorkerThreads();
}

void UWorkerThreadsSubsystem::EnqueueTasks(const TArray<TFunction<void()>>& Tasks)
{
	if (TaskQueues.Num() == 0) return;
	
	// Distribute tasks across worker queues using round-robin
	static int32 QueueIndex = 0;
	
	for (const auto& Task : Tasks)
	{
		TaskQueues[QueueIndex % TaskQueues.Num()]->Enqueue(Task);
		TaskEvents[QueueIndex % TaskEvents.Num()]->Trigger();
		QueueIndex++;
	}

}


void UWorkerThreadsSubsystem::InitializeWorkerThreads()
{
	const int32 NumOfThreads = FPlatformMisc::NumberOfCores();
	
	for (int i = 0; i < NumOfThreads; i++)
	{
		// Create a separate queue and event for each worker
		TQueue<TFunction<void()>, EQueueMode::Mpsc>* Queue = new TQueue<TFunction<void()>, EQueueMode::Mpsc>();
		FEvent* Event = FPlatformProcess::GetSynchEventFromPool(false);
		
		TaskQueues.Add(Queue);
		TaskEvents.Add(Event);
		
		FWorker* Worker = new FWorker(*Queue, Event);
		FString ThreadName = FString::Printf(TEXT("WorkerThread_%i"), i);
		FRunnableThread* Thread = FRunnableThread::Create(Worker, *ThreadName, 0, TPri_AboveNormal);
		Workers.Add(Worker);
		WorkerThreads.Add(Thread);
	}


	UE_LOG(LogTemp, Display, TEXT("%d Worker Threads Created"), NumOfThreads);

	bShouldRun = true;
	bProcessMainThread = true;

	RunSendLoop();
	RunReceiveLoop();
}

void UWorkerThreadsSubsystem::StopAllProcesses()
{
	bProcessMainThread = false;

	for (FWorker* Worker : Workers)
	{
		Worker->Stop();
	}

	for (FRunnableThread* Thread : WorkerThreads)
	{
		if (Thread)
		{
			Thread->Kill(true);
			delete Thread;
		}
	}
	
	// Clean up queues and events
	for (const auto* Queue : TaskQueues)
	{
		delete Queue;
	}
	for (auto* Event : TaskEvents)
	{
		FPlatformProcess::ReturnSynchEventToPool(Event);
	}
	
	Workers.Empty();
	WorkerThreads.Empty();
	TaskQueues.Empty();
	TaskEvents.Empty();

}

void UWorkerThreadsSubsystem::RunSendLoop() const
{
	// Use a dedicated OS thread instead of a long-lived UE::Tasks task to avoid starving the task graph
	Async(EAsyncExecution::Thread, [this]()
	{
		while (bProcessMainThread)
		{
			bool bProcessedMessage = false;

			if (GameSessionSubsystem->HasPendingOutgoingMessages())
			{
				ProcessOutgoingMessages();
				bProcessedMessage = true;
			}

			// Only sleep if no message was processed
			if (!bProcessedMessage)
			{
				FPlatformProcess::YieldThread();
			}
		}
	});
}

void UWorkerThreadsSubsystem::RunReceiveLoop()
{
	// Use a dedicated OS thread instead of a long-lived UE::Tasks task to avoid starving the task graph
	Async(EAsyncExecution::Thread, [this]()
	{
		while (bProcessMainThread)
		{
			bool bProcessedMessage = false;

			if(GameSessionSubsystem->HasPendingIncomingMessages())
			{
				ProcessIncomingMessages();
				bProcessedMessage = true;
			}

			// Only sleep if no message was processed
			if (!bProcessedMessage)
			{
				FPlatformProcess::YieldThread();
			}
		}
	});
}

void UWorkerThreadsSubsystem::ProcessOutgoingMessages() const
{
	if (TArray<uint8>* SendBuffer = BufferPoolSubsystem->GetBuffer())
	{
		GameSessionSubsystem->DequeueMessageToSend(*SendBuffer);
		if (!UDPSubsystem->SendUDPMessage(*SendBuffer))
		{
			BufferPoolSubsystem->ReleaseBuffer(SendBuffer);
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Buffer Pool exhausted."));
	}
}

void UWorkerThreadsSubsystem::ProcessIncomingMessages()
{
	TArray<TFunction<void()>> Batch;
	Batch.Reserve(50);

	for (int i = 0; i < 50; ++i)
	{
		Batch.Add([this]()
		{
			// First Get A Buffer
			if (TArray<uint8>* ReceiveBuffer = BufferPoolSubsystem->GetBuffer())
			{
				// Check if there is actually a message to receive
				if (GameSessionSubsystem->DequeueMessageToReceive(*ReceiveBuffer))
				{
					//UE_LOG(LogTemp, Log, TEXT("Worker executing task"));
					NetworkMessageParser->ParseMessage(*ReceiveBuffer);
				}
				else
				{
					BufferPoolSubsystem->ReleaseBuffer(ReceiveBuffer);
					
				}
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("Buffer Pool exhausted."));
			}
		});
	}

	if (Batch.Num() > 0)
	{
		EnqueueTasks(Batch);
	}
}
