// Fill out your copyright notice in the Description page of Project Settings.
#include "Network/Services/GameData/VoxelServiceSubsystem.h"

#include "FunctionLibraries/Network/FL_Serialization.h"
#include "Shared/Types/Structures/Voxels/FChunkVoxelState.h"
#include "Network/Infrastructure/NetworkMessageParser.h"
#include "Network/GraphQL/GraphQLService.h"
#include "Network/Services/Core/UDPSubsystem.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"
#include "Voxels/Data/VoxelDataManager.h"
#include "Voxels/Rendering/ChunkDataManager.h"


DEFINE_LOG_CATEGORY(LogVoxelService);

void UVoxelServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UVoxelServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UVoxelServiceSubsystem::PostSubsystemInit()
{
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
	VoxelDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelDataManager>();
	GraphQLService = GetWorld()->GetGameInstance()->GetSubsystem<UGraphQLService>();
	ChunkDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UChunkDataManager>();

	if (!GameSessionSubsystem || !UDPSubsystem || !VoxelDataManager || !GraphQLService)
	{
		UE_LOG(LogVoxelService, Error, TEXT("Some or one of the subsystem is invalid."));
		return;
	}

	UE_LOG(LogVoxelService, Log, TEXT("Voxel Service Subsystem Initialized."));
}

void UVoxelServiceSubsystem::SendVoxelListRequest(int64 X, int64 Y, int64 Z, int32 UnixTimestamp)
{
	if (!IsValid(GraphQLService))
	{
		UE_LOG(LogVoxelService, Warning, TEXT("Game Instance is not valid."));
		return;
	}

	TMap<FString, FString> QueryVariables;

	QueryVariables.Add(TEXT("mapId"), FString::Printf(TEXT("%lld"), GameSessionSubsystem->GetMapID()));
	QueryVariables.Add(TEXT("x"), FString::Printf(TEXT("%lld"), X));
	QueryVariables.Add(TEXT("y"), FString::Printf(TEXT("%lld"), Y));
	QueryVariables.Add(TEXT("z"), FString::Printf(TEXT("%lld"), Z));

	GraphQLService->ExecuteQueryByID(EGraphQLQuery::VoxelList, QueryVariables);
}

void UVoxelServiceSubsystem::SendVoxelStateUpdateRequest(const int64 Cx, const int64 Cy, const int64 Cz, const int32 Vx, const int32 Vy, const int32 Vz,
                                                         const uint8 VoxelType, const FVoxelState VoxelState, const bool bSendState)
{
	TArray<uint8> Payload;

	// Add MapID
	Payload.Append(UFL_Serialization::SerializeValue(GameSessionSubsystem->GetMapID()));
	
	// Add Chunk Coordinates
	Payload.Append(UFL_Serialization::SerializeValue(Cx));
	Payload.Append(UFL_Serialization::SerializeValue(Cy));
	Payload.Append(UFL_Serialization::SerializeValue(Cz));
	
	const int16 x = FMath::Clamp(Vx, -32768, 32767);
	const int16 y = FMath::Clamp(Vy, -32768, 32767);
	const int16 z = FMath::Clamp(Vz, -32768, 32767);

	UE_LOG(LogTemp, Log, TEXT("Voxel Update Request for coords %d, %d, %d"), x, y, z);

	Payload.Append(UFL_Serialization::SerializeValue(x));
	Payload.Append(UFL_Serialization::SerializeValue(y));
	Payload.Append(UFL_Serialization::SerializeValue(z));
	
	// Append Voxel Type
	const int16 VoxelTypeInt = static_cast<int16>(VoxelType);
	Payload.Append(UFL_Serialization::SerializeValue(VoxelTypeInt));

	if (bSendState)
	{
		// Append Voxel State
		TArray<uint8> VoxelStateBytes = VoxelState.SerializeToBytes();
		Payload.Append(VoxelStateBytes);
	}
	
	
	if (UDPSubsystem->QueueUDPMessage(EMessageType::VOXEL_UPDATE_REQUEST, Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("Voxel State Update Request Sent through UDP."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Voxel State Update Request failed to send."));
	}
}

void UVoxelServiceSubsystem::HandleNewVoxelUpdateNotification(const TArray<uint8>& Payload) const
{
	const int PayloadLength = Payload.Num();

	if (PayloadLength < 0)
	{
		UE_LOG(LogTemp, Log, TEXT("New Voxel Update Notification Failed. Payload size too small."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("New Voxel Update Notification Received. Payload size: %d"), PayloadLength);

	int32 Offset = 0;
	int64 MapId;
	int64 ChunkX, ChunkY, ChunkZ;

	UFL_Serialization::DeserializeValue(Payload, MapId, Offset);
	Offset += sizeof(int64);
	
	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(int64);
	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(int64);
	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(int64);

	int16 Vx, Vy, Vz;
	UFL_Serialization::DeserializeValue(Payload, Vx, Offset);
	Offset += sizeof(int16);
	UFL_Serialization::DeserializeValue(Payload, Vy, Offset);
	Offset += sizeof(int16);
	UFL_Serialization::DeserializeValue(Payload, Vz, Offset);
	Offset += sizeof(int16);

	
	
	// Deserialize VoxelType (1 byte)
	uint8 VoxelType = Payload[Offset];
	Offset += sizeof(int16);

	bool bHasState = false;
	FVoxelState VoxelState;
	VoxelState.ResetVoxelState();
	
	// Check if there's enough data for both VoxelState and mapID
	if (PayloadLength > 72)
	{
		bHasState = true;
		VoxelState.DeserializeFromBytes(Payload, Offset);
	}
	
	
	// Dispatch the call for voxel update on the game thread.
	UE_LOG(LogTemp, Log, TEXT("Voxel Update Response for coords %d, %d, %d"), Vx, Vy, Vz);
	UE_LOG(LogTemp, Log, TEXT("Voxel Update Response Voxel Type: %d"), VoxelType);
	
	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ChunkX, ChunkY, ChunkZ, Vx, Vy, Vz, VoxelType, VoxelState, bHasState]
	{
		OnNewVoxelUpdateNotification.Broadcast(ChunkX, ChunkY, ChunkZ, Vx, Vy, Vz, VoxelType, VoxelState, bHasState);
	}, TStatId(), nullptr, ENamedThreads::GameThread);
	
}

void UVoxelServiceSubsystem::HandleNewVoxelListResponse(const TArray<uint8>& Payload)
{
	int32 const PayloadLength = Payload.Num();
    //UE_LOG(LogTemp, Log, TEXT("Voxel List Response Payload Length: %d"), payloadLength);

	// Discard handling if Payload does not contain enough bytes
	if (PayloadLength < 5)
	{
		UE_LOG(LogVoxelService, Error,
		       TEXT("HandleNewVoxelListResponse: Payload too short to process. Received size:%d, Expected Size >= 38"),
		       PayloadLength);
		return;
	}
	
	int64 ChunkX, ChunkY, ChunkZ;
	int32 Timestamp;

	int32 Offset = 0;

	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(ChunkX);
	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(ChunkY);
	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(ChunkZ);

	//UE_LOG(LogVoxelService, Log, TEXT("HandleNewVoxelListResponse: Chunk Coords: %lld, %lld, %lld"), ChunkX, ChunkY, ChunkZ);
	
	UFL_Serialization::DeserializeValue(Payload, Timestamp, Offset);
	Offset += sizeof(Timestamp);

	//UE_LOG(LogVoxelService, Log, TEXT("HandeNewVoxelListResponse: Timestamp: %d"), Timestamp);

	EErrorCode ErrorCode = static_cast<EErrorCode>(Payload[Offset]);
	Offset += sizeof(EErrorCode);

	if (ErrorCode != EErrorCode::SUCCESS)
	{
		UE_LOG(LogVoxelService, Warning, TEXT("HandleNewVoxelListResponse: Error received: %d"), static_cast<int32>(ErrorCode));
		VoxelDataManager->OnVoxelListResponse(false, {}, {}, {}, {} );
		return;
	}
	
	uint16 VoxelStatesLength;
	UFL_Serialization::DeserializeValue(Payload, VoxelStatesLength, Offset);
	Offset += sizeof(VoxelStatesLength);

	//UE_LOG(LogVoxelService, Log, TEXT("HandleNewVoxelListResponse: Voxel States Length: %d"), VoxelStatesLength)

	TArray<FChunkVoxelState> VoxelStates;
	const int32 StartOffset = Offset;
	const int32 EndOffset = StartOffset + VoxelStatesLength;

	if (EndOffset > PayloadLength)
	{
		//UE_LOG(LogVoxelService, Error, TEXT("HandleNewVoxelListResponse: Payload too short for voxel states"));
		VoxelDataManager->OnVoxelListResponse(true, ChunkX, ChunkY, ChunkZ, {});
		return;
	}

	//UE_LOG(LogVoxelService, Log, TEXT("HandleNewVoxelListResponse: StartOffset: %d, EndOffset: %d"), StartOffset, EndOffset)

	while (Offset < EndOffset)
	{
		if (Offset + 4 > PayloadLength)
		{
			UE_LOG(LogTemp, Error, TEXT("HandleNewVoxelListResponse: Payload too short for voxel state"));
			break;
		}

		uint8 x, y, z, VoxelType;
		UFL_Serialization::DeserializeValue(Payload, x, Offset);
		Offset += sizeof(x);
		UFL_Serialization::DeserializeValue(Payload, y, Offset);
		Offset += sizeof(y);
		UFL_Serialization::DeserializeValue(Payload, z, Offset);
		Offset += sizeof(z);
		UFL_Serialization::DeserializeValue(Payload, VoxelType, Offset);
		Offset += sizeof(VoxelType);

		uint16 VoxelStateLength;
		UFL_Serialization::DeserializeValue(Payload, VoxelStateLength, Offset);
		Offset += sizeof(VoxelStateLength);

		//UE_LOG(LogVoxelService, Log, TEXT("HandleNewVoxelListResponse: Each Voxel State Length: %d, expected Voxel State Length:%llu"),VoxelStateLength, sizeof(FVoxelState) )

		FChunkVoxelState ChunkVoxelState;
		ChunkVoxelState.Vx = x;
		ChunkVoxelState.Vy = y;
		ChunkVoxelState.Vz = z;
		ChunkVoxelState.VoxelType = VoxelType;
		//UE_LOG(LogVoxelService, Log, TEXT("HandleNewVoxelListResponse: All Voxel Data: Coords: %d, %d, %d, Type: %d"), ChunkVoxelState.Vx,
		//		   ChunkVoxelState.Vy, ChunkVoxelState.Vz, ChunkVoxelState.VoxelType);

		if (VoxelStateLength > 0)
		{
			FVoxelState VoxelState;
			// Check version matching and load the state 
			if (Payload[Offset] == 1)
			{
				VoxelState.DeserializeFromBytes(Payload, Offset);
			}
			ChunkVoxelState.VoxelState = VoxelState;
			Offset += VoxelStateLength;

			//UE_LOG(LogTemp, Log, TEXT("HandleNewVoxelListResponse: Voxel State: %d, %lld, %d"),
			//	   ChunkVoxelState.VoxelState.Rotation, ChunkVoxelState.VoxelState.AtlasOverride,
			//	   ChunkVoxelState.VoxelState.FaceOneDirection);
		}
		VoxelStates.Add(ChunkVoxelState);
	}

	//UE_LOG(LogVoxelService, Log, TEXT("HandleNewVoxelListResponse: Total Voxel States Extracted for Chunk:%lld, %lld, %lld : %d"), ChunkX, ChunkY, ChunkZ, VoxelStates.Num());

	VoxelDataManager->OnVoxelListResponse(true, ChunkX, ChunkY, ChunkZ, VoxelStates);
}

void UVoxelServiceSubsystem::HandleVoxelUpdateResponse(const TArray<uint8>& Payload) const
{
	int const PayloadLength = Payload.Num();
	
	if(PayloadLength <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Voxel Update Response Error: Payload size too small."));
		return;
	}

	int32 Offset = 0;
	int64 MapId;

	UFL_Serialization::DeserializeValue(Payload, MapId, Offset);
	Offset += sizeof(int64);
	
	int64 ChunkX, ChunkY, ChunkZ;
	
	UFL_Serialization::DeserializeValue(Payload, ChunkX, Offset);
	Offset += sizeof(int64);
	UFL_Serialization::DeserializeValue(Payload, ChunkY, Offset);
	Offset += sizeof(int64);
	UFL_Serialization::DeserializeValue(Payload, ChunkZ, Offset);
	Offset += sizeof(int64);

	int16 Vx, Vy, Vz;
	UFL_Serialization::DeserializeValue(Payload, Vx, Offset);
	Offset += sizeof(int16);
	UFL_Serialization::DeserializeValue(Payload, Vy, Offset);
	Offset += sizeof(int16);
	UFL_Serialization::DeserializeValue(Payload, Vz, Offset);
	Offset += sizeof(int16);

	UE_LOG(LogTemp, Log, TEXT("Voxel Update Response for coords %d, %d, %d"), Vx, Vy, Vz);

	if (Offset < Payload.Num()) {
		const EErrorCode ErrorCode = static_cast<EErrorCode>(Payload[Offset]);
		UE_LOG(LogTemp, Log, TEXT("Voxel Update Response Error Code: %hhd"), ErrorCode);
	}

	FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ChunkX, ChunkY, ChunkZ, Vx, Vy, Vz]
	{
		OnVoxelUpdateResponse.Broadcast(ChunkX, ChunkY, ChunkZ, Vx, Vy, Vz);
	}, TStatId(), nullptr, ENamedThreads::GameThread);


	// Skipping HMAC
	//Offset += 32;

	// Skipping GameTokenID
	Offset += 8;
	
}

void UVoxelServiceSubsystem::HandleVoxelListGraphQLResponse(const TSharedPtr<FJsonObject>& Payload) const
{
	if (!Payload.IsValid())
	{
		UE_LOG(LogVoxelService, Warning, TEXT("Invalid payload."));
		VoxelDataManager->OnVoxelListResponse(false, {}, {}, {}, {});
		return;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* VoxelListObject;

	// Extract data object first
	if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogVoxelService, Error, TEXT("Failed to extract data object from response"));
		return;
	}

	if (!(*DataObject)->TryGetObjectField(TEXT("getVoxelList"), VoxelListObject))
	{
		UE_LOG(LogVoxelService, Error, TEXT("Failed to extract getVoxelList from json"));
		return;
	}

	// Extract coordinates from data
	int64 X, Y, Z;
	if (!UFL_Serialization::ExtractChunkCoordinates(*VoxelListObject, X, Y, Z))
	{
		UE_LOG(LogVoxelService, Error, TEXT("Failed to extract chunk coordinates from response json"));
		return;
	}

	// Extract voxel list from data
	const TArray<TSharedPtr<FJsonValue>>* VoxelListArray;

	if (!(*VoxelListObject)->TryGetArrayField(TEXT("voxels"), VoxelListArray))
	{
		UE_LOG(LogVoxelService, Error, TEXT("Failed to extract voxel list from json"));
		//VoxelDataManager->OnVoxelListResponse(false, X, Y, Z, {});
		return;
	}

	TArray<FChunkVoxelState> VoxelStates;
	FChunkDataContainer DataContainer;
	
	for (auto& VoxelData : *VoxelListArray)
	{
		const TSharedPtr<FJsonObject>* VoxelStateObj;
		VoxelData->TryGetObject(VoxelStateObj);

		const TSharedPtr<FJsonObject>* Coordinates;
		if (!(*VoxelStateObj)->TryGetObjectField(TEXT("location"), Coordinates))
		{
			UE_LOG(LogVoxelService, Warning, TEXT("Coordinates not found in voxel list element object"));
			continue;
		}

		FString sX, sY, sZ, sType, sState;
		if (!(*Coordinates)->TryGetStringField(TEXT("x"), sX) ||
			!(*Coordinates)->TryGetStringField(TEXT("y"), sY) ||
			!(*Coordinates)->TryGetStringField(TEXT("z"), sZ) ||
			!(*VoxelStateObj)->TryGetStringField(TEXT("voxelType"), sType))
		{
			UE_LOG(LogVoxelService, Warning, TEXT("Failed to extract core voxel list item(s)"));
		}
		
		uint8 cx, cy, cz, VoxelType;
		cx = static_cast<uint8>(FCString::Atoi(*sX));
		cy = static_cast<uint8>(FCString::Atoi(*sY));
		cz = static_cast<uint8>(FCString::Atoi(*sZ));
		VoxelType = static_cast<uint8>(FCString::Atoi(*sType));


		FVoxelState VoxelState; // default-initialized

		TArray<uint8> DecodedState;

		bool bStateFound = false;
		
		if (!(*VoxelStateObj)->TryGetStringField(TEXT("state"), sState))
		{
			UE_LOG(LogVoxelService, Warning, TEXT("Failed to extract voxel state"));
			bStateFound = false;
		}
		else
		{
			bStateFound = true;
		}

		if (bStateFound)
		{
			if (FBase64::Decode(sState, DecodedState))
			{
				if (DecodedState.Num() >= sizeof(FVoxelState))
				{
					VoxelState.DeserializeFromBytes(DecodedState);
				}
				else
				{
					UE_LOG(LogVoxelService, Warning, TEXT("Decoded state too small, using default"));
				}
			}
			else
			{
				UE_LOG(LogVoxelService, Warning, TEXT("Failed to decode voxel state, using default"));
			}
		}
		
		FChunkVoxelState ChunkVoxelState;
		ChunkVoxelState.Vx = cx;
		ChunkVoxelState.Vy = cy;
		ChunkVoxelState.Vz = cz;
		ChunkVoxelState.VoxelType = VoxelType;
		ChunkVoxelState.VoxelState = VoxelState;
		VoxelStates.Add(ChunkVoxelState);

		DataContainer.VoxelStatesMap.Add(FVoxelCoordinate(cx, cy, cz), FVoxelDefinition(1, VoxelType, VoxelState));
	}
	
	ChunkDataManager->OnVoxelListDataReceived(true, FInt64Vector(X,Y,Z), DataContainer);
	//VoxelDataManager->OnVoxelListResponse(true, X, Y, Z, VoxelStates);
}
