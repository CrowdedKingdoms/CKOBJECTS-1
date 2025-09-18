// Fill out your copyright notice in the Description page of Project Settings.

#include "CKNetwork/Pubilc/Network/Services/GameData/ActorServiceSubsystem.h"
#include "EngineUtils.h"
#include "CKNetwork/Pubilc/Network/Services/Core/UDPSubsystem.h"
#include "CKTypes/Public/Shared/Types/Core/GameSessionSubsystem.h"
#include "CKNetwork/Pubilc/FunctionLibraries/Network/FL_Serialization.h"
#include "CKPlayer/Public/Player/NonAuthClients/NPC_Manager.h"
#include "CKNetwork/Pubilc/Network/Infrastructure/NetworkMessageParser.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"
#include "Shared/Types/Structures/Actors/FActorUpdateStruct.h"

void UActorServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UActorServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UActorServiceSubsystem::SendActorUpdate(const int64 ChunkX, const int64 ChunkY, const int64 ChunkZ,
                                             const FString UUID,
                                             const FActorState& State)
{
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, ChunkX, ChunkY, ChunkZ, UUID, State]()
	{
		TArray<uint8> Payload;

		// Append the Map ID
		Payload.Append(UFL_Serialization::SerializeValue(GameSessionSubsystem->GetMapID()));

		// Append the Chunks
		Payload.Append(UFL_Serialization::SerializeValue(ChunkX));
		Payload.Append(UFL_Serialization::SerializeValue(ChunkY));
		Payload.Append(UFL_Serialization::SerializeValue(ChunkZ));

		//Serialize and append the UUID
		const FTCHARToUTF8 ConvertedUUID(*UUID);
		Payload.Append(reinterpret_cast<const uint8*>(ConvertedUUID.Get()), ConvertedUUID.Length());
		
		//Finally, Append the state itself
		Payload.Append(State.SerializeActorState(State));
		
		// Send the message with the appropriate message type
		if (UDPSubsystem->QueueUDPMessage(EMessageType::ACTOR_UPDATE_REQUEST, Payload))
		{
			//UE_LOG(LogTemp, Log, TEXT("ACTOR_UPDATE_REQ sent: %s, Seq: %u, Time: %.6f"), 
			//	   *UUID, SequenceNum, CurrentTime);
			// UE_LOG(LogTemp, Log, TEXT("ACTOR_UPDATE_REQ sent: %s"), *UUID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ACTOR_UPDATE_REQ not sent: %s"), *UUID);
		}
	}, UE::Tasks::ETaskPriority::BackgroundNormal);
}

void UActorServiceSubsystem::HandleActorUpdateNotification(const TArray<uint8>& Payload) const
{
	if (Payload.Num() < (sizeof(int64) * 3 + 32 + sizeof(int32)))
	{
		UE_LOG(LogTemp, Error, TEXT("Payload too small to contain required data."));
		return;
	}

	int32 Offset = 0;

	Offset+=sizeof(int64); //skip mapID

	FActorUpdateStruct UpdateInfo;

	int64 ChunkX;
	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(int64);

	int64 ChunkY;
	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(int64);

	int64 ChunkZ;
	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(int64);

	FString UUID = UFL_Serialization::DeserializeString(Payload, Offset, 32);
	Offset += 32;
	
	FActorState ActorState;
	ActorState = ActorState.DeserializeActorState(Payload, Offset);
	Offset += sizeof(FActorState);
	

	UpdateInfo.ChunkX = ChunkX;
	UpdateInfo.ChunkY = ChunkY;
	UpdateInfo.ChunkZ = ChunkZ;
	UpdateInfo.UUID = UUID;
	UpdateInfo.State = ActorState;

	//UE_LOG(LogTemp, Log, TEXT("Actor Update Notify: UUID %s for chunk %lld, %lld, %lld"), *UUID, ChunkX, ChunkY, ChunkZ);
	ProcessActorUpdateInfo(UpdateInfo);
	
}

void UActorServiceSubsystem::HandleActorUpdateResponse(const TArray<uint8>& Payload) const
{
	int64 MapId, ChunkX, ChunkY, ChunkZ;

	int32 Offset = 0;

	// Extract Map ID
	UFL_Serialization::DeserializeValue(Payload, MapId, Offset);
	Offset += sizeof(MapId);

	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(ChunkX);

	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(ChunkY);

	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(ChunkZ);

	// Extract UUID
	const FString UUID = UFL_Serialization::DeserializeString(Payload, Offset, 32);
	Offset += 32;

	const EErrorCode ErrorCode = static_cast<EErrorCode>(Payload[Offset]);

	UE_LOG(LogTemp, Log, TEXT("Actor Update Response: UUID %s for chunk %lld, %lld, %lld"), *UUID, ChunkX, ChunkY, ChunkZ);
	UE_LOG(LogTemp, Error, TEXT("Error received for Actor %s with error code %d"), *UUID, ErrorCode);
}

void UActorServiceSubsystem::PostSubsystemInit()
{
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();

	if (!UDPSubsystem || !GameSessionSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Actor Service: Subsystem(s) reference is invalid."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Actor Service Subsystem Initialized"));
}

void UActorServiceSubsystem::ProcessActorUpdateInfo(const FActorUpdateStruct& UpdateInfo) const
{
	if (!NPC_Manager) return;
	
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateInfo]()
	{
		NPC_Manager->ProcessUpdates(UpdateInfo);
	}, UE::Tasks::ETaskPriority::BackgroundNormal);
}

