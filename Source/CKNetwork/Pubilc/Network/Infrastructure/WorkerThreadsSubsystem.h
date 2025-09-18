// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "WorkerThreadsSubsystem.generated.h"

class FWorker;
class UUDPSubsystem;
class UGameSessionSubsystem;
class UGameServiceManager;
class UNetworkMessageParser;
class UMessageBufferPoolSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskCompleted);
DECLARE_LOG_CATEGORY_EXTERN(ThreadPoolLog, Log, All);
/**
 * This class manages Thread Pools for worker functions to perform tasks.
 */
UCLASS(BlueprintType)
class CROWDEDKINGDOMS_API UWorkerThreadsSubsystem : public UGameInstanceSubsystem, public  ISubsystemInitializable
{
	GENERATED_BODY()
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Worker Threads Subsystem")
	virtual void PostSubsystemInit() override;

	void EnqueueTasks(const TArray<TFunction<void()>>& Tasks);

private:

	void InitializeWorkerThreads();
	
	void StopAllProcesses();
	
	void RunSendLoop() const;

	void RunReceiveLoop();

	void ProcessOutgoingMessages() const;

	void ProcessIncomingMessages();

	bool bProcessMainThread = false;
	bool bShouldRun = false;
	bool bWasPreviouslyConnected = false;
	
	UPROPERTY()
	UMessageBufferPoolSubsystem* BufferPoolSubsystem;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;

	UPROPERTY()
	UNetworkMessageParser* NetworkMessageParser;

	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;

	
	TArray<FRunnableThread*> WorkerThreads;
	
	TArray<FWorker*> Workers;

	TArray<TQueue<TFunction<void()>, EQueueMode::Mpsc>*> TaskQueues;
	TArray<FEvent*> TaskEvents;
	
};



