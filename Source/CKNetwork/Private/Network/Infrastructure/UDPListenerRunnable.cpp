// Fill out your copyright notice in the Description page of Project Settings.


#include "CKNetwork/Pubilc/Network/Infrastructure/UDPListenerRunnable.h"
#include "Sockets.h"
#include "CKNetwork/Pubilc/Network/Services/Core/UDPSubsystem.h"
#include "SocketSubsystem.h"


FUDPListenerRunnable::FUDPListenerRunnable(FSocket* InSocket, UUDPSubsystem* InOwner) : Socket(InSocket),
	Owner(InOwner), bRun(true)
{
	Buffer.SetNumUninitialized(1280);
}

FUDPListenerRunnable::~FUDPListenerRunnable()
{
}

bool FUDPListenerRunnable::Init()
{
	return (Socket != nullptr);
}

uint32 FUDPListenerRunnable::Run()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem || !Socket)
	{
		return 0;
	}

	TSharedRef<FInternetAddr> ClientAddr = SocketSubsystem->CreateInternetAddr();

	while (bRun)
	{
		
		// Drain everything in the socket buffer
		int32 BytesRead = 0;
		if (Socket->RecvFrom(Buffer.GetData(), Buffer.Num(), BytesRead, *ClientAddr))
		{
			if (BytesRead > 0 && Owner)
			{
				// Pass directly without copy (Owner enqueues to its queue)
				Owner->HandleUDPMessage(Buffer.GetData(), BytesRead, *ClientAddr);
			}
		}
		else
		{
			const ESocketErrors Error = SocketSubsystem->GetLastErrorCode();
			if (Error != SE_EWOULDBLOCK && Error != SE_NO_ERROR && Error != SE_EINTR)
			{
				UE_LOG(LogTemp, Error, TEXT("UDP RecvFrom error: %d"), (int32)Error);
				return 0; // Fatal
			}
		}
		
		FPlatformProcess::YieldThread();
	}
	return 0;
}


void FUDPListenerRunnable::Stop()
{
	bRun = false;
}

void FUDPListenerRunnable::Exit()
{
	FRunnable::Exit();
}
