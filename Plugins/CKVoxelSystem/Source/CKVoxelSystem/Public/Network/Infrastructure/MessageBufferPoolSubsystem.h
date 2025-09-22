// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Containers/Queue.h"
#include "HAL/CriticalSection.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "MessageBufferPoolSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(BufferPoolLog, Log, All);


/*
 * This class is used to create, handle and release Message Buffer pools for networking.
 */

UCLASS(BlueprintType)
class  UMessageBufferPoolSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Message Buffer Pool Subsystem")
	virtual void PostSubsystemInit() override;
	

	UFUNCTION()
	void InitializeBufferPool(int32 InMaxBufferPoolSize, int32 InBufferSize);

	
	TArray<uint8>* GetBuffer();

	void ReleaseBuffer(TArray<uint8>* BufferToRelease);



private:

	UPROPERTY()
	int32 DefaultBufferSize;

	UPROPERTY()
	int32 MaxPoolSize;

	UPROPERTY()
	int32 CurrentBufferCount;

	
	TArray<TArray<uint8>*> BufferPool;
	FCriticalSection BufferPoolLock;

	static TArray<uint8>* AllocateBuffer(int32 BufferSize);
};

