// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/Services/GameData/GameObjectsServiceSubsystem.h"

#include "FunctionLibraries/Network/FL_Serialization.h"
#include "GameObjects/Framework/Management/GameObjectsManager.h"
#include "Network/Services/Core/UDPSubsystem.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"
#include "Shared/Types/Enums/Events/EEventType.h"


void UGameObjectsServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	//TODO: Set GameSessionSubsystem reference
}

void UGameObjectsServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UGameObjectsServiceSubsystem::PostSubsystemInit()
{
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();

	if (!UDPSubsystem || !GameSessionSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Some or one of the subsystem is invalid."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Game Objects Service Subsystem Initialized."));
}

void UGameObjectsServiceSubsystem::SendGameObjectActivationRequest(const int64 MapId, const int64 ChunkX,
                                                                   const int64 ChunkY, const int64 ChunkZ, const FString& ActivatorUUID, const uint16 EventType,
                                                                   const FGameObjectState& State) const
{
	// Start with an empty payload
	TArray<uint8> Payload;

	// Append Map ID
	const int64 MapID = GameSessionSubsystem->GetMapID();
	Payload.Append(UFL_Serialization::SerializeValue(MapID));

	// Append Chunk Data
	Payload.Append(UFL_Serialization::SerializeValue(ChunkX));
	Payload.Append(UFL_Serialization::SerializeValue(ChunkY));
	Payload.Append(UFL_Serialization::SerializeValue(ChunkZ));

	// Append Activator UUID
	const FTCHARToUTF8 ConvertedUUID(*ActivatorUUID);
	Payload.Append(reinterpret_cast<const uint8*>(ConvertedUUID.Get()), ConvertedUUID.Length());

	// Append the event type
	Payload.Append(UFL_Serialization::SerializeValue(EventType));

	// Append the state size and state itself
	//constexpr int32 StateSize = sizeof(FGameObjectState);
	//Payload.Append(UFL_Serialization::SerializeValue(StateSize));

	TArray<uint8> StateBytes;
	StateBytes.SetNumUninitialized(sizeof(FGameObjectState));

	FMemory::Memcpy(StateBytes.GetData(), &State, sizeof(FGameObjectState));
	Payload.Append(StateBytes);
	
	UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Sending GameObject Activation Request"));

	UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Sending State is: %s"), State.bIsActive ? TEXT("True"): TEXT("False"));
	//Dispatch with UDP Service
	if (!UDPSubsystem->QueueUDPMessage(EMessageType::CLIENT_EVENT_NOTIFICATION, Payload))
	{
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Failed to send GameObject Activation Request"));
	}
}

void UGameObjectsServiceSubsystem::HandleGameObjectActivationNotification(const TArray<uint8>& Payload) const
{
	// Payload Validation
	if (Payload.Num() < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Invalid payload size :%d"), Payload.Num());
		return;
	}
	
	int32 Offset = 0;

	// Extract Map ID
	int64 MapId;
	UFL_Serialization::DeserializeValue(Payload, MapId, Offset);
	Offset += sizeof(MapId);

	// Extract Chunk Coords
	int64 ChunkX, ChunkY, ChunkZ;
	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(ChunkX);
	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(ChunkY);
	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(ChunkZ);

	// Extract Activator UUID
	FString ActivatorUUID = UFL_Serialization::DeserializeString(Payload, Offset, 32);
	Offset += 32;

	// Extract Event Type
	Offset += sizeof(uint16);

	// Extract State Size
	//Offset += sizeof(int32);

	// Extract State
	FGameObjectState State;
	FMemory::Memcpy(&State, Payload.GetData() + Offset, sizeof(FGameObjectState));
	
	
	UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Received GameObject Activation Notification. Dispatching to GameObjectsManager"));

	UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Received State is: %s"), State.bIsActive ? TEXT("True"): TEXT("False"));
	//Activate The Object
	AsyncTask(ENamedThreads::GameThread, [this, ActivatorUUID, State]()
	{
		GameObjectsManager->OnGameObjectActivationNotificationReceived.Broadcast(ActivatorUUID, State);
	});
}

void UGameObjectsServiceSubsystem::SendTriggerBallEventRequest(const int64 ChunkX, const int64 ChunkY, const int64 ChunkZ, const FString InUUID, const FBallState BallState)
{
	TArray<uint8> Payload;

	// Append Map ID
	const int64 MapID = GameSessionSubsystem->GetMapID();
	Payload.Append(UFL_Serialization::SerializeValue(MapID));

	// Append Chunk Data
	Payload.Append(UFL_Serialization::SerializeValue(ChunkX));
	Payload.Append(UFL_Serialization::SerializeValue(ChunkY));
	Payload.Append(UFL_Serialization::SerializeValue(ChunkZ));

	// Event UUID 
	const FTCHARToUTF8 ConvertedUUID(*InUUID);
	Payload.Append(reinterpret_cast<const uint8*>(ConvertedUUID.Get()), ConvertedUUID.Length());

	//Event Type
	constexpr uint16 EventType = static_cast<uint16>(EEventType::Ball);
	Payload.Append(UFL_Serialization::SerializeValue(EventType));

	// Event State Size
	//constexpr int32 StateSize = sizeof(FBallState);
	//Payload.Append(UFL_Serialization::SerializeValue(StateSize));
	
	// Prepare and append ball state
	TArray<uint8> BallStateBytes;
	BallStateBytes.SetNumUninitialized(sizeof(FBallState));
	
	FMemory::Memcpy(BallStateBytes.GetData(), &BallState, sizeof(FBallState));
	Payload.Append(BallStateBytes);
	
	UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Sending Trigger Ball Event Request"));
	
	if (!UDPSubsystem->QueueUDPMessage(EMessageType::CLIENT_EVENT_NOTIFICATION, Payload))
	{
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Failed to send Trigger Ball Event Request"));
	}
}

void UGameObjectsServiceSubsystem::HandleTriggerBallEventNotification(const TArray<uint8>& Payload) const
{
	// Payload Validation
	if (Payload.Num() < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Invalid payload size :%d"), Payload.Num());
		return;
	}

	int32 Offset = 0;

	// Extract Map ID
	int64 MapId;
	UFL_Serialization::DeserializeValue(Payload, MapId, Offset);
	Offset += sizeof(MapId);

	// Extract Chunk Coords
	int64 ChunkX, ChunkY, ChunkZ;
	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(ChunkX);
	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(ChunkY);
	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(ChunkZ);

	// Extract UUID
	FString UUID = UFL_Serialization::DeserializeString(Payload, Offset, 32);
	Offset += 32;

	// Skip Event Type
	Offset += sizeof(uint16);

	// Skip State Size
	//Offset += sizeof(int32);

	FBallState BallState;
	
	if (Offset + sizeof(FBallState) <= Payload.Num())
	{
		FMemory::Memcpy(&BallState, Payload.GetData() + Offset, sizeof(FBallState));
		if (GameObjectsManager)
		{
			AsyncTask(ENamedThreads::GameThread, [this, UUID, BallState]()
			{
				GameObjectsManager->OnBallEventReceived.Broadcast(UUID, BallState);
			});
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Payload too small to contain FBallState data"));
	}

	UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: GameObjects Manager reference invalid"));
}

void UGameObjectsServiceSubsystem::HandleGameEventNotification(const TArray<uint8>& Payload) const
{
	const int32 PayloadSize = Payload.Num();

	if (PayloadSize <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Invalid payload size :%d"), PayloadSize);
		return;
	}

	// We get the event type directly
	int32 Offset = 0;
	Offset += sizeof(int64); //skip mapID

	// Skip Chunk Coordinates
	Offset += sizeof(int64); //skip chunkX
	Offset += sizeof(int64); //skip ChunkY
	Offset += sizeof(int64); //skip ChunkZ

	// Skip UUID
	Offset += 32; //skip UUID

	// Get the event type
	uint16 EventType;
	UFL_Serialization::DeserializeValue(Payload, EventType, Offset);

	// Cast to EEventType
    EEventType TypedEvent = static_cast<EEventType>(EventType);

	switch (TypedEvent)
	{
		case EEventType::Ball:
		UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Ball Event Received."));
		HandleTriggerBallEventNotification(Payload);
		break;

		case EEventType::Door:
		UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Door Event Received"));
		HandleGameObjectActivationNotification(Payload);
		break;

		case EEventType::Light:
		UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Light Event Received"));
		HandleGameObjectActivationNotification(Payload);
		break;

		case EEventType::Checkpoint:
		UE_LOG(LogTemp, Log, TEXT("Service_GameObjectService: Checkpoint Event Received"));
		HandleGameObjectActivationNotification(Payload);
		break;
		
		default:
		UE_LOG(LogTemp, Error, TEXT("Service_GameObjectService: Unknown Event Type"));
		break;
	}
	
}
