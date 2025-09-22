// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


class  FWorker : public FRunnable
{
public:
	
	FWorker(TQueue<TFunction<void()>, EQueueMode::Mpsc>& InQueue, FEvent* InTaskEvent);
	virtual ~FWorker() override;

	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;



private:

	FThreadSafeBool bShouldStop;

	TQueue<TFunction<void()>, EQueueMode::Mpsc>& TaskQueue;
	FEvent* TaskEvent;
	
	
};
