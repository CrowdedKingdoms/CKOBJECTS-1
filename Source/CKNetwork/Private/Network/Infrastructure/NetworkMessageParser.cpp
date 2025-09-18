// Fill out your copyright notice in the Description page of Project Settings.
#include "CKNetwork/Pubilc/Network/Infrastructure/NetworkMessageParser.h"
#include "CKTypes/Public/Shared/Types/Enums/Network/MessageType.h"
#include "Engine/World.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/GameObjectsServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Infrastructure/MessageBufferPoolSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/ActorServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/ChunkServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/VoxelServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/Communication/TextChatServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/Communication/VoiceChatServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/User/UserServiceSubsystem.h"

DEFINE_LOG_CATEGORY(NetworkMessageParser);

void UNetworkMessageParser::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UNetworkMessageParser::Deinitialize()
{
	Super::Deinitialize();
}


void UNetworkMessageParser::PostSubsystemInit()
{
	UserServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUserServiceSubsystem>();
	ChunkServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UChunkServiceSubsystem>();
	CDNServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCDNServiceSubsystem>();
	VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	ActorServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UActorServiceSubsystem>();
	VoiceChatServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoiceChatServiceSubsystem>();
	TextChatServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTextChatServiceSubsystem>();
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
	GameObjectServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameObjectsServiceSubsystem>();
	BufferPoolSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UMessageBufferPoolSubsystem>();


	if (!UserServiceSubsystem || !ChunkServiceSubsystem || !CDNServiceSubsystem || !VoxelServiceSubsystem || !
		ActorServiceSubsystem || !VoiceChatServiceSubsystem || !TextChatServiceSubsystem || !UDPSubsystem || !
		GameObjectServiceSubsystem || !BufferPoolSubsystem)
	{
		UE_LOG(NetworkMessageParser, Error, TEXT("Subsystem reference is invalid."));
	}
}


void UNetworkMessageParser::ParseMessage(TArray<uint8>& Message) const
{
	FString ResponseString;

	if (!BufferPoolSubsystem)
	{
		UE_LOG(NetworkMessageParser, Error, TEXT("Buffer Pool is not set."));
		return;
	}

	if (Message.Num() < 1)
	{
		UE_LOG(NetworkMessageParser, Error, TEXT("Message is too small to contain a header."));
		return;
	}
	
	const TArray<uint8> Payload(Message.GetData() + 1, Message.Num() - 1);

	// Parse and handle the message
	const EMessageType MsgType = static_cast<EMessageType>(Message[0]);
	BufferPoolSubsystem->ReleaseBuffer(&Message);
	switch (MsgType)
	{
	// UDP Messages
	// new types -- start -- 
	case EMessageType::ACTOR_UPDATE_NOTIFICATION:
		//UE_LOG(NetworkMessageParser, Log, TEXT("Actor Update Notification Received."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			ActorServiceSubsystem->HandleActorUpdateNotification(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
		break;

	case EMessageType::ACTOR_UPDATE_RESPONSE:
		UE_LOG(NetworkMessageParser, Log, TEXT("Actor Update Response Received."))
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			ActorServiceSubsystem->HandleActorUpdateResponse(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
		break;

	case EMessageType::VOXEL_UPDATE_NOTIFICATION:
		UE_LOG(NetworkMessageParser, Log, TEXT("Voxel Update Notification UDP Received."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			VoxelServiceSubsystem->HandleNewVoxelUpdateNotification(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
		break;

	case EMessageType::VOXEL_UPDATE_RESPONSE:
		UE_LOG(NetworkMessageParser, Log, TEXT("Voxel Update Response UDP Received."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			VoxelServiceSubsystem->HandleVoxelUpdateResponse(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
		break;

	case EMessageType::CLIENT_AUDIO_NOTIFICATION:
		//UE_LOG(NetworkService, Log, TEXT("Client Audio Notification Received."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			VoiceChatServiceSubsystem->HandleClientAudioNotification(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
		break;

	case EMessageType::CLIENT_TEXT_NOTIFICATION:
		UE_LOG(NetworkMessageParser, Log, TEXT("Client Text Notification Received."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			TextChatServiceSubsystem->HandleIncomingTextChatMessage(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundLow);
		break;

	case EMessageType::CLIENT_EVENT_NOTIFICATION:
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, Payload]()
		{
			GameObjectServiceSubsystem->HandleGameEventNotification(Payload);
		}, LowLevelTasks::ETaskPriority::BackgroundNormal);
		break;

	// Default Case (Unknown Message)
	default:
		UE_LOG(NetworkMessageParser, Warning, TEXT("Unknown message type received: %s"), *ResponseString);
		break;
	}
}
