// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UUDPSubsystem;
/**
 * 
 */
class FUDPListenerRunnable : public FRunnable
{
public:
	FUDPListenerRunnable(FSocket* InSocket, UUDPSubsystem* InOwner);
	virtual ~FUDPListenerRunnable() override;

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

private:
	FSocket* Socket;
	UUDPSubsystem* Owner;
	FThreadSafeBool bRun;
	TArray<uint8> Buffer;
};
