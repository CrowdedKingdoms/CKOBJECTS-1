// Fill out your copyright notice in the Description page of Project Settings.


#include "Threads/FWorker.h"


FWorker::FWorker(TQueue<TFunction<void()>, EQueueMode::Mpsc>& InQueue, FEvent* InTaskEvent)
	: bShouldStop(false)   // bind reference here
	, TaskQueue(InQueue)
	, TaskEvent(InTaskEvent)
{
}

FWorker::~FWorker()
{
}

uint32 FWorker::Run()
{
	while (!bShouldStop)
	{
		// Process any available tasks first
		TFunction<void()> Task;
		bool bFoundTask = false;
		
		while (TaskQueue.Dequeue(Task))
		{
			if (Task)
			{
				Task();
				bFoundTask = true;
			}
			if (bShouldStop) break;
		}
		
		// Only wait if no tasks were found
		if (!bFoundTask && !bShouldStop)
		{
			TaskEvent->Wait();
		}
	}
	return 0;

}

void FWorker::Stop()
{
	bShouldStop = true;
	if (TaskEvent)
	{
		TaskEvent->Trigger();
	}
}

void FWorker::Exit()
{
	FRunnable::Exit();
}
