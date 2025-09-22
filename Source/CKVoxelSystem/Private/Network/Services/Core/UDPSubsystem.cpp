#include "Network/Services/Core/UDPSubsystem.h"

#include <chrono>

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "FunctionLibraries/Network/FL_Serialization.h"
#include "Network/Infrastructure/NetworkMessageParser.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"
#include "Network/Infrastructure/MessageBufferPoolSubsystem.h"
#include "Network/Infrastructure/UDPListenerRunnable.h"
#include "Shared/Types/Enums/Network/MessageType.h"


DEFINE_LOG_CATEGORY(LogUDPService);

void UUDPSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUDPSubsystem::Deinitialize()
{
	StopUDPListener();
	StopUDPv4Listener();
	CleanupSockets();
	Super::Deinitialize();
}

void UUDPSubsystem::PostSubsystemInit()
{
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	BufferPoolSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMessageBufferPoolSubsystem>();
	
	if (!GameSessionSubsystem || !BufferPoolSubsystem)
	{
		UE_LOG(LogUDPService, Error, TEXT("GameSessionSubsystem is invalid. Returning early..."));
		return;
	}

	UE_LOG(LogUDPService, Log, TEXT("GameSessionSubsystem is valid. Initializing UDP Service."));
}


FUDPNetworkStats UUDPSubsystem::GetUDPNetworkStats() const
{
	FUDPNetworkStats Stats;
	Stats.BytesSent = BytesSent;
	Stats.BytesReceived = BytesReceived;
	Stats.MessagesSent = MessagesSent;
	Stats.MessagesReceived = MessagesReceived;
	return Stats;
}

void UUDPSubsystem::StartTimeoutMonitoring(const float ThresholdSeconds)
{
	TimeoutThresholdSeconds = ThresholdSeconds;
	bTimeoutEnabled = true;

	LastMessageTime = std::chrono::steady_clock::now();
	
	if (const UWorld* World = GetWorld())
	{
		// Check for timeout every 5 seconds (you can adjust this)
		World->GetTimerManager().SetTimer(
			TimeoutCheckTimerHandle,
			this,
			&UUDPSubsystem::CheckForTimeout,
			5.0f, // Check every 5 seconds
			true  // Loop
		);
        
		UE_LOG(LogUDPService, Log, TEXT("UDP timeout monitoring started with threshold: %.2f seconds"), ThresholdSeconds);
	}

}

void UUDPSubsystem::StopTimeoutMonitoring()
{
	bTimeoutEnabled = false;
    
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeoutCheckTimerHandle);
		UE_LOG(LogUDPService, Log, TEXT("UDP timeout monitoring stopped"));
	}

}

float UUDPSubsystem::GetTimeSinceLastMessage() const
{
	if (!bTimeoutEnabled)
	{
		return 0.0f;
	}
    
	const auto CurrentTime = std::chrono::steady_clock::now();
	const auto LastMsgTime = LastMessageTime.load();
    
	const auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(
		CurrentTime - LastMsgTime);
    
	return Duration.count() / 1000.0f; // Convert to seconds

}

void UUDPSubsystem::HandleUDPMessage(const uint8* Data, const int32 Size, const FInternetAddr& Addr)
{
	if (Size <= 0)
	{
		return;
	}

	// Update Network Stats
	this->BytesReceived += Size;
	MessagesReceived++;
	OnMessageReceived();
	
	
	TArray<uint8> Message;
	Message.SetNumUninitialized(Size);
	FMemory::Memcpy(Message.GetData(), Data, Size);
	GameSessionSubsystem->EnqueueMessageToReceive(MoveTemp(Message));
}

void UUDPSubsystem::CheckForTimeout()
{
	if (!bTimeoutEnabled)
	{
		return;
	}
    
	const auto CurrentTime = std::chrono::steady_clock::now();
	const auto LastMsgTime = LastMessageTime.load();
    
	const auto TimeSinceLastMessage = std::chrono::duration_cast<std::chrono::seconds>(
		CurrentTime - LastMsgTime).count();
    
	if (TimeSinceLastMessage >= TimeoutThresholdSeconds)
	{
		UE_LOG(LogUDPService, Warning, TEXT("UDP timeout detected - no messages received for %lld seconds"), 
			   TimeSinceLastMessage);
        
		// Stop monitoring to prevent multiple timeout triggers
		StopTimeoutMonitoring();
        
		// Stop all UDP operations
		StopAllUDPOperations();
        
		// Broadcast the timeout event
		OnUDPTimeout.Broadcast();
	}
}

void UUDPSubsystem::OnMessageReceived()
{
	// This is thread-safe - can be called from any thread
	if (bTimeoutEnabled)
	{
		LastMessageTime = std::chrono::steady_clock::now();
	}
}

void UUDPSubsystem::StopAllUDPOperations()
{
	UE_LOG(LogUDPService, Log, TEXT("Stopping all UDP operations due to timeout"));
    
	// Stop listeners
	StopUDPListener();
	StopUDPv4Listener();
    
	// Stop timeout monitoring
	StopTimeoutMonitoring();
    
	// Clean up sockets
	CleanupSockets();
    
	// Clear any pending timers
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UDPStatsTimerHandle);
	}

}

bool UUDPSubsystem::QueueUDPMessage(const EMessageType MessageType, const TArray<uint8>& Data) const
{
	// Return if Send Socket not Initialized
	if (!UDPSocket && !UDPSocketV4)
	{
		return false;
	}
	
	// Get UniqueID
	TArray<uint8> GameTokenID_Bytes;
	const int64 GameTokenID = GameSessionSubsystem->GetGameTokenID();
	GameTokenID_Bytes.SetNumUninitialized(8);
	FMemory::Memcpy(GameTokenID_Bytes.GetData(), &GameTokenID, 8);
	
	// Prepare Header
	TArray<uint8> Lcl_MessageType;

	Lcl_MessageType.SetNumUninitialized(1);
	Lcl_MessageType[0] = static_cast<uint32>(MessageType) & 0xFF;
	
	// Append Header
	TArray<uint8> Message;
	Message.Append(Lcl_MessageType);
	Message.Append(Data);

	// Get HMAC
	const TArray<uint8> HMAC = UFL_Serialization::CalculateHMAC(Message, GameSessionSubsystem->GetGameToken());

	// Append Message
	if (Message.Num() != 0)
	{
		Message.Append(HMAC);
		Message.Append(GameTokenID_Bytes);
	}
	
	// Message Sending 
	const bool bMessageQueued = GameSessionSubsystem->EnqueueMessageToSend(Message);

	return bMessageQueued;
}

bool UUDPSubsystem::SendUDPMessage(TArray<uint8>& Message)
{
	bool bMessageSent;
	
	if (bUseIPv4) // IPv4
	{
		bMessageSent = SendUDPv4(Message);
		BufferPoolSubsystem->ReleaseBuffer(&Message);
		return bMessageSent;
	}
	
	bMessageSent = SendUDPv6(Message);

	if (!bMessageSent)
	{
		UE_LOG(LogUDPService, Warning, TEXT("Failed to send UDP message over IPv6. Switching to IPv4."));
		SwitchToIPv4();
		BufferPoolSubsystem->ReleaseBuffer(&Message);
		return bMessageSent = SendUDPv4(Message);
	}
	
	BufferPoolSubsystem->ReleaseBuffer(&Message);
	return bMessageSent;
}

bool UUDPSubsystem::SendUDPv6(const TArray<uint8>& Message) const
{
	if (!UDPSocket)
	{
		UE_LOG(LogUDPService, Error, TEXT("UDP Socket (IPv6) is null, cannot send message"));
		return false;
	}

	int32 BytesSentNow = 0;
	bool bUDPPacketSent = UDPSocket->Send(Message.GetData(), Message.Num(), BytesSentNow);

	if (bUDPPacketSent && BytesSentNow > 0)
	{
		// Update stats (need to cast away const to modify member variables)
		BytesSent += BytesSentNow;
		MessagesSent++;
	}
	else
	{
		// Get socket subsystem for error reporting
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSubsystem)
		{
			ESocketErrors SocketError = SocketSubsystem->GetLastErrorCode();

			switch (SocketError)
			{
			case SE_NO_ERROR:
				UE_LOG(LogUDPService, Warning, TEXT("UDP (IPv6) packet send completed but no bytes sent"));
				break;
			case SE_EWOULDBLOCK:
				UE_LOG(LogUDPService, Warning, TEXT("UDP (IPv6) send would block - socket buffer full"));
				break;
			case SE_ECONNRESET:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv6) connection reset by peer"));
				break;
			case SE_ENETDOWN:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv6) network is down"));
				break;
			case SE_EHOSTUNREACH:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv6) host unreachable"));
				break;
			case SE_EMSGSIZE:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv6) message too large"));
				break;
			default:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv6) packet send failed with socket error: %d"),
				       (int32)SocketError);
			}
		}
		else
		{
			UE_LOG(LogUDPService, Error, TEXT("UDP (IPv6) packet send failed - unable to get error details"));
		}
	}

	return bUDPPacketSent && BytesSentNow > 0;
}

bool UUDPSubsystem::SendUDPv4(const TArray<uint8>& Message) const
{
	if (!UDPSocketV4)
	{
		UE_LOG(LogUDPService, Error, TEXT("UDP Socket (IPv4) is null, cannot send message"));
		return false;
	}

	int32 BytesSentNow = 0;
	bool bUDPPacketSent = UDPSocketV4->Send(Message.GetData(), Message.Num(), BytesSentNow);

	if (bUDPPacketSent && BytesSentNow > 0)
	{
		// Update stats (need to cast away const to modify member variables)
		BytesSent += BytesSentNow;
		MessagesSent++;
	}
	else
	{
		// Get socket subsystem for error reporting
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSubsystem)
		{
			ESocketErrors SocketError = SocketSubsystem->GetLastErrorCode();

			switch (SocketError)
			{
			case SE_NO_ERROR:
				UE_LOG(LogUDPService, Warning, TEXT("UDP (IPv4) packet send completed but no bytes sent"));
				break;
			case SE_EWOULDBLOCK:
				UE_LOG(LogUDPService, Warning, TEXT("UDP (IPv4) send would block - socket buffer full"));
				break;
			case SE_ECONNRESET:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv4) connection reset by peer"));
				break;
			case SE_ENETDOWN:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv4) network is down"));
				break;
			case SE_EHOSTUNREACH:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv4) host unreachable"));
				break;
			case SE_EMSGSIZE:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv4) message too large"));
				break;
			default:
				UE_LOG(LogUDPService, Error, TEXT("UDP (IPv4) packet send failed with socket error: %d"),
				       (int32)SocketError);
			}
		}
		else
		{
			UE_LOG(LogUDPService, Error, TEXT("UDP (IPv4) packet send failed - unable to get error details"));
		}
	}

	return bUDPPacketSent && BytesSentNow > 0;
}

void UUDPSubsystem::HandleUDPAddressNotification(const TSharedPtr<FJsonObject>& Payload)
{
	if (!Payload.IsValid())
	{
		UE_LOG(LogUDPService, Error, TEXT("Payload is invalid. Returning early..."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(false);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}

	const TSharedPtr<FJsonObject> DataObject = Payload->GetObjectField(TEXT("data"));

	if (!DataObject.IsValid())
	{
		UE_LOG(LogUDPService, Error, TEXT("Data object is invalid in payload. Returning early..."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(false);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}

	const TSharedPtr<FJsonObject> ServerObject = DataObject->GetObjectField(TEXT("serverWithLeastClients"));
	if (!ServerObject.IsValid())
	{
		UE_LOG(LogUDPService, Error, TEXT("Server object is invalid in payload. Returning early..."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(false);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}

	FString IPv6;
	FString IPv4;
	uint16 Port;

	if (!ServerObject->TryGetStringField(TEXT("ip6"), IPv6))
	{
		UE_LOG(LogUDPService, Error, TEXT("IPv6 address is invalid in payload. Trying ipv4"));
	}

	if (!ServerObject->TryGetStringField(TEXT("ip4"), IPv4))
	{
		UE_LOG(LogUDPService, Error, TEXT("IPv4 address is invalid in payload. Returning early..."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(false);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}

	if (!ServerObject->TryGetNumberField(TEXT("clientPort"), Port))
	{
		UE_LOG(LogUDPService, Error, TEXT("Port is invalid in payload. Returning early..."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(false);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}


	//UE_LOG(LogUDPService, Log, TEXT("Received UDP IPv6 address %s and port %hu, IPv4 address %s and port %hu"), *IPv6,
	//      Port, *IPv4, Port);


	const bool bInitIPv6 = InitializeUDPSocket(IPv6, Port);
	const bool bInitIPv4 = InitializeV4UDPSocket(IPv4, Port);

	if (bInitIPv6 && !bUseIPv4)
	{
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(true);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}

	if (bInitIPv4)
	{
		SwitchToIPv4();
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(true);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
	}
	else
	{
		UE_LOG(LogUDPService, Error, TEXT("Failed to initialize UDP socket!"));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnUDPEndpointSet.Broadcast(false);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
	}
}

void UUDPSubsystem::SwitchToIPv4()
{
	bUseIPv4 = true;
	StopUDPListener();

	// Start the IPv4 listener in a background thread
	AsyncTask(
		ENamedThreads::AnyBackgroundThreadNormalTask,
		[this]()
		{
			UE_LOG(LogUDPService, Log, TEXT("UDP IPv4 Listen Loop Started."));
			StartUDPv4Listener();
		});
}

void UUDPSubsystem::CleanupSockets()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (UDPSocket)
	{
		UDPSocket->Close();
		SocketSubsystem->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
	}

	if (UDPSocketV4)
	{
		UDPSocketV4->Close();
		SocketSubsystem->DestroySocket(UDPSocketV4);
		UDPSocketV4 = nullptr;
	}
	
}

bool UUDPSubsystem::InitializeUDPSocket(const FString& IPAddress, const int32 Port)
 {
 	// Get socket subsystem
 	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
 	if (!SocketSubsystem)
 	{
 		UE_LOG(LogUDPService, Error, TEXT("Failed to get socket subsystem"));
 		return false;
 	}
 
 	// Create a target address to determine an IP version
 	const TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
 	bool bIsValidAddress = false;
 	TargetAddr->SetIp(*IPAddress, bIsValidAddress);
 
 	if (!bIsValidAddress)
 	{
 		UE_LOG(LogUDPService, Error, TEXT("Invalid IP address: %s"), *IPAddress);
 		return false;
 	}
 
 	TargetAddr->SetPort(Port);
 
 	// Determine if this is IPv4 or IPv6 based on the target address
 	const bool bIsIPv6 = TargetAddr->GetProtocolType() == FNetworkProtocolTypes::IPv6;
 	const FName ProtocolType = bIsIPv6 ? FNetworkProtocolTypes::IPv6 : FNetworkProtocolTypes::IPv4;
 
 	// Create a UDP socket with the correct protocol
 	UDPSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDP Socket"), ProtocolType);
 	if (!UDPSocket)
 	{
 		UE_LOG(LogUDPService, Error, TEXT("Failed to create UDP socket"));
 		return false;
 	}
 
 	// Create local address with matching protocol
 	const TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->CreateInternetAddr(ProtocolType);
	
	// Set any appropriate address based on protocol
	if (bIsIPv6)
	{
		// For IPv6, we need to set the any address differently
		bool bIsValid = false;
		LocalAddr->SetIp(TEXT("::"), bIsValid);  // IPv6 any address
		if (!bIsValid)
		{
			UE_LOG(LogUDPService, Error, TEXT("Failed to set IPv6 any address"));
			CleanupSockets();
			return false;
		}
	}
	
 	LocalAddr->SetPort(0); // Let the system assign a local port
 
 	// Bind socket to local address
 	if (!UDPSocket->Bind(*LocalAddr))
 	{
 		UE_LOG(LogUDPService, Error, TEXT("Failed to bind UDP socket to local address"));
 		// Cleanup socket inline
 		CleanupSockets();
 		return false;
 	}
 
 	// Connect to the target address (for UDP, this sets the default destination)
 	if (!UDPSocket->Connect(*TargetAddr))
 	{
 		UE_LOG(LogUDPService, Error, TEXT("Failed to connect UDP socket to %s:%d"), *IPAddress, Port);
 		// Cleanup socket inline
 		CleanupSockets();
 		return false;
 	}
 
 	// Set socket to non-blocking mode
 	UDPSocket->SetNonBlocking(true);
	int32 NewSendBufferSize = 0;
	int32 NewRecvBufferSize = 0;
	UDPSocket->SetSendBufferSize(16000000, NewSendBufferSize);
	UDPSocket->SetReceiveBufferSize(16000000, NewRecvBufferSize);
	UE_LOG(LogUDPService, Log, TEXT("UDP (%s) buffers set: snd=%d, rcv=%d"),
		bIsIPv6 ? TEXT("IPv6") : TEXT("IPv4"), NewSendBufferSize, NewRecvBufferSize);
	
 	UE_LOG(LogUDPService, Log, TEXT("Successfully connected UDP socket (%s) to %s:%d"), 
 		bIsIPv6 ? TEXT("IPv6") : TEXT("IPv4"), *IPAddress, Port);

	ListenerRunnable = new FUDPListenerRunnable(UDPSocket, this);
	ListenerThread = FRunnableThread::Create(ListenerRunnable, TEXT("UDPListenerThread"), 0, TPri_AboveNormal);
	
	// // Start the listener in a background thread
 	// AsyncTask(
 	// 	ENamedThreads::AnyBackgroundThreadNormalTask,
 	// 	[this]()
 	// 	{
 	// 		UE_LOG(LogUDPService, Log, TEXT("UDP Listen Loop Started."));
 	// 		StartUDPListener();
 	// 	});
 
 	// Start stats timer on game thread
 	AsyncTask(
 		ENamedThreads::GameThread,
 		[this]()
 		{
 			if (const UWorld* World = GetWorld())
 			{
 				World->GetTimerManager().SetTimer(UDPStatsTimerHandle, this, &UUDPSubsystem::UpdateUDPNetworkStats,
 				                                  1.0f, true);
 			}
 		});
 
 	return true;
 }

bool UUDPSubsystem::InitializeV4UDPSocket(const FString& IPAddress, const int32 Port)
{
	// Get socket subsystem
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogUDPService, Error, TEXT("Failed to get socket subsystem"));
		return false;
	}

	// Create UDP socket specifically for IPv4
	UDPSocketV4 = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDP Socket V4"), false);
	if (!UDPSocketV4)
	{
		UE_LOG(LogUDPService, Error, TEXT("Failed to create UDP socket (IPv4)"));
		return false;
	}

	// Create target address for connection (IPv4)
	const TSharedRef<FInternetAddr> TargetAddr = SocketSubsystem->CreateInternetAddr();
	bool bIsValidAddress = false;
	TargetAddr->SetIp(*IPAddress, bIsValidAddress);

	if (!bIsValidAddress)
	{
		UE_LOG(LogUDPService, Error, TEXT("Invalid IPv4 address: %s"), *IPAddress);
		// Cleanup socket inline
		CleanupSockets();
		return false;
	}

	TargetAddr->SetPort(Port);

	// Create local address for binding (IPv4)
	TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->CreateInternetAddr();
	LocalAddr->SetAnyAddress(); // Bind to any IPv4 address
	LocalAddr->SetPort(0); // Let system assign a local port

	// Allow socket reuse
	UDPSocketV4->SetReuseAddr(true);

	// Bind socket to local address
	if (!UDPSocketV4->Bind(*LocalAddr))
	{
		UE_LOG(LogUDPService, Error, TEXT("Failed to bind UDP socket (IPv4) to local address"));
		// Cleanup socket inline
		CleanupSockets();
		return false;
	}

	// Connect to target address (for UDP, this sets the default destination)
	if (!UDPSocketV4->Connect(*TargetAddr))
	{
		UE_LOG(LogUDPService, Error, TEXT("Failed to connect UDP socket (IPv4) to %s:%d"), *IPAddress, Port);
		// Cleanup socket inline
		CleanupSockets();
		return false;
	}

	// Set socket to non-blocking mode
	UDPSocketV4->SetNonBlocking(true);
	int32 NewSendBufferSize = 0;
	int32 NewRecvBufferSize = 0;
	UDPSocketV4->SetSendBufferSize(16000000, NewSendBufferSize);
	UDPSocketV4->SetReceiveBufferSize(16000000, NewRecvBufferSize);
	UE_LOG(LogUDPService, Log, TEXT("UDP (IPv4) buffers set: snd=%d, rcv=%d"), NewSendBufferSize, NewRecvBufferSize);

	// Get local address info for logging
	const TSharedRef<FInternetAddr> BoundAddr = SocketSubsystem->CreateInternetAddr();
	UDPSocketV4->GetAddress(*BoundAddr);
	if (BoundAddr->IsValid())
	{
		const FString LocalIP = BoundAddr->ToString(false); // false = don't include port
		const int32 LocalPort = BoundAddr->GetPort();
		UE_LOG(LogUDPService, Log, TEXT("UDP Socket (IPv4) bound locally to [%s]:%d"), *LocalIP, LocalPort);
	}
	else
	{
		UE_LOG(LogUDPService, Warning, TEXT("Unable to retrieve local address for IPv4 socket"));
	}

	UE_LOG(LogUDPService, Log, TEXT("UDP Socket (IPv4) Created and Ready for Server [%s]:%d"), *IPAddress, Port);

	return true;
}

void UUDPSubsystem::StartUDPListener()
{
	bUDPListen = true;

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(1280);

	// Socket should already be set to non-blocking in initialization
	if (UDPSocket)
	{
		UDPSocket->SetNonBlocking(true);
	}

	while (bUDPListen)
	{
		bool bProcessedPacket = false;
		
		// Check if the socket has data available
		uint32 PendingDataSize;
		if (UDPSocket && UDPSocket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
		{
			// Get the socket subsystem for address creation
			ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
			if (!SocketSubsystem)
			{
				UE_LOG(LogUDPService, Error, TEXT("Failed to get socket subsystem in listener"));
				break;
			}

			// Create address to receive from
			TSharedRef<FInternetAddr> ClientAddr = SocketSubsystem->CreateInternetAddr();

			int32 bytesReceived = 0;

			// Receive data from socket
			if (UDPSocket->RecvFrom(Buffer.GetData(), Buffer.Num(), bytesReceived, *ClientAddr))
			{
				if (bytesReceived > 0)
				{
					bProcessedPacket = true;
					
					// Update Network Stats
					this->BytesReceived += bytesReceived;
					MessagesReceived++;

					OnMessageReceived();
					
					// Create a temp data buffer with an exact size
					TArray<uint8> Data;
					Data.Append(Buffer.GetData(), bytesReceived);

					// Log message type (first byte)
					if (Data.Num() > 0)
					{
						uint8 MessageType = Data[0];
						UE_LOG(LogUDPService, Log, TEXT("Received UDP message type: %d"), MessageType);
					}

					// Get client address info for logging
					FString ClientIP = ClientAddr->ToString(false); // false = don't include port
					int32 ClientPort = ClientAddr->GetPort();
					
					if (UFL_Serialization::AuthenticateHMAC(Data, GameSessionSubsystem->GetGameToken()))
					{
						// Enqueue message for processing
						if (GameSessionSubsystem)
						{
							GameSessionSubsystem->EnqueueMessageToReceive(Data);
						}
					}
					else
					{
						UE_LOG(LogUDPService, Warning, TEXT("HMAC Authentication Failed for message type %d"), Data[0]);
					}
				}
			}
			else
			{
				// Handle receive error
				const ESocketErrors SocketError = SocketSubsystem->GetLastErrorCode();

				switch (SocketError)
				{
				case SE_NO_ERROR:
					// No error, but no data received
					break;
				case SE_EWOULDBLOCK:
					// No data available right now, continue
					break;
				case SE_ECONNRESET:
					UE_LOG(LogUDPService, Error, TEXT("Connection reset by peer"));
					break;
				case SE_ENETDOWN:
					UE_LOG(LogUDPService, Error, TEXT("Network subsystem failed"));
					break;
				case SE_EINTR:
					UE_LOG(LogUDPService, Error, TEXT("Receive operation interrupted"));
					break;
				case SE_EMSGSIZE:
					UE_LOG(LogUDPService, Error, TEXT("Message too large for UDP buffer"));
					break;
				case SE_ETIMEDOUT:
					UE_LOG(LogUDPService, Error, TEXT("Connection timed out"));
					break;
				default:
					UE_LOG(LogUDPService, Error, TEXT("Receive failed with socket error: %d"), (int32)SocketError);
				}

				// For fatal errors, break the loop
				if (SocketError != SE_EWOULDBLOCK && SocketError != SE_EINTR && SocketError != SE_NO_ERROR)
				{
					break;
				}
			}
		}

		// Only sleep if no packets were processed
		if (!bProcessedPacket)
		{
			FPlatformProcess::Sleep(0.001f); // 1 ms sleep
		}
	}

	UE_LOG(LogUDPService, Log, TEXT("UDP Listener Loop Terminated"));
}

void UUDPSubsystem::StopUDPListener()
{
	bUDPListen = false;

	if (ListenerRunnable)
	{
		ListenerRunnable->Stop();
	}
	
	if (ListenerThread)
	{
		ListenerThread->Kill(true);
		delete ListenerThread;
		ListenerThread = nullptr;
	}

	if (ListenerRunnable)
	{
		delete ListenerRunnable;
		ListenerRunnable = nullptr;
	}
	
	UE_LOG(LogUDPService, Log, TEXT("UDP IPv6 Listener Loop Stopped"));
}

void UUDPSubsystem::StartUDPv4Listener()
{
	bUDPv4Listen = true;

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(1280);

	// Socket should already be set to non-blocking in initialization
	if (UDPSocketV4)
	{
		UDPSocketV4->SetNonBlocking(true);
	}

	while (bUDPv4Listen)
	{
		bool bProcessedPacket = false;
		
		// Check if socket has data available
		uint32 PendingDataSize = false;
		if (UDPSocketV4 && UDPSocketV4->HasPendingData(PendingDataSize) && PendingDataSize > 0)
		{
			// Get the socket subsystem for address creation
			ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
			if (!SocketSubsystem)
			{
				UE_LOG(LogUDPService, Error, TEXT("Failed to get socket subsystem in IPv4 listener"));
				break;
			}

			// Create address to receive from (IPv4)
			TSharedRef<FInternetAddr> ClientAddr = SocketSubsystem->CreateInternetAddr();

			int32 bytesReceived = 0;

			// Receive data from socket
			if (UDPSocketV4->RecvFrom(Buffer.GetData(), Buffer.Num(), bytesReceived, *ClientAddr))
			{
				if (bytesReceived > 0)
				{
					bProcessedPacket = true;
					
					// Update Network Stats
					this->BytesReceived += bytesReceived;
					MessagesReceived++;

					OnMessageReceived();
					
					// Create a temp data buffer with exact size
					TArray<uint8> Data;
					Data.Append(Buffer.GetData(), bytesReceived);

					// Log message type (first byte)
					if (Data.Num() > 0)
					{
						uint8 MessageType = Data[0];
						UE_LOG(LogUDPService, Log, TEXT("Received UDP IPv4 message type: %d"), MessageType);
					}

					// Remove HMAC (assuming 32 bytes at the end)
					if (Data.Num() > 32)
					{
						Data.SetNum(bytesReceived - 32);
					}

					// Get client address info for logging
					FString ClientIP = ClientAddr->ToString(false); // false = don't include port
					int32 ClientPort = ClientAddr->GetPort();
					
					if (UFL_Serialization::AuthenticateHMAC(Buffer, GameSessionSubsystem->GetGameToken()))
					{
						// Enqueue message for processing
						if (GameSessionSubsystem)
						{
							GameSessionSubsystem->EnqueueMessageToReceive(Data);
						}
					}
				}
			}
			else
			{
				// Handle receive error
				ESocketErrors SocketError = SocketSubsystem->GetLastErrorCode();

				switch (SocketError)
				{
				case SE_NO_ERROR:
					// No error, but no data received
					break;
				case SE_EWOULDBLOCK:
					// No data available right now, continue
					break;
				case SE_ECONNRESET:
					UE_LOG(LogUDPService, Error, TEXT("IPv4 connection reset by peer"));
					break;
				case SE_ENETDOWN:
					UE_LOG(LogUDPService, Error, TEXT("IPv4 network subsystem failed"));
					break;
				case SE_EINTR:
					UE_LOG(LogUDPService, Error, TEXT("IPv4 receive operation interrupted"));
					break;
				case SE_EMSGSIZE:
					UE_LOG(LogUDPService, Error, TEXT("IPv4 message too large for UDP buffer"));
					break;
				case SE_ETIMEDOUT:
					UE_LOG(LogUDPService, Error, TEXT("IPv4 connection timed out"));
					break;
				default:
					UE_LOG(LogUDPService, Error, TEXT("IPv4 receive failed with socket error: %d"), (int32)SocketError);
				}

				// For fatal errors, break the loop
				if (SocketError != SE_EWOULDBLOCK && SocketError != SE_EINTR && SocketError != SE_NO_ERROR)
				{
					break;
				}
			}
		}

		// Only sleep if no packets were processed
		if (!bProcessedPacket)
		{
			FPlatformProcess::Sleep(0.001f); // 1 ms sleep
		}
	}

	UE_LOG(LogUDPService, Log, TEXT("UDP IPv4 Listener Loop Terminated"));
}

void UUDPSubsystem::StopUDPv4Listener()
{
	bUDPv4Listen = false;
	UE_LOG(LogUDPService, Log, TEXT("UDP IPv4 Listener Loop Stopped"));
}

void UUDPSubsystem::UpdateUDPNetworkStats()
{
	// Reset counters every second
	BytesSent = 0;
	BytesReceived = 0;
	MessagesSent = 0;
	MessagesReceived = 0;
}
