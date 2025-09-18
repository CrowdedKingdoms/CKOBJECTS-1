// Fill out your copyright notice in the Description page of Project Settings.
#include "CKNetwork/Pubilc/Network/Services/GameData/ChunkServiceSubsystem.h"

#include "CKNetwork/Pubilc/FunctionLibraries/Network/FL_Serialization.h"
#include "Shared/Types/Structures/Voxels/FChunkVoxelState.h"
#include "CKNetwork/Pubilc/Network/GraphQL/GraphQLService.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"
#include "CKVoxelSystem/Public/Voxels/Data/VoxelDataManager.h"
#include "Shared/Types/Structures/Chunks/FChunkDataContainer.h"
#include "Shared/Types/Structures/Voxels/FChunkData.h"
#include "CKVoxelSystem/Public/Voxels/Rendering/ChunkDataManager.h"

DEFINE_LOG_CATEGORY(LogChunkService);

void UChunkServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UChunkServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UChunkServiceSubsystem::PostSubsystemInit()
{
	CDNServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCDNServiceSubsystem>();
	VoxelDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelDataManager>();
	GraphQLService = GetWorld()->GetGameInstance()->GetSubsystem<UGraphQLService>();
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	ChunkDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UChunkDataManager>();

	if (!CDNServiceSubsystem || !VoxelDataManager || !GraphQLService || !GameSessionSubsystem)
	{
		UE_LOG(LogChunkService, Error, TEXT("Some or one of the subsystem is invalid."));
		return;
	}

	UE_LOG(LogChunkService, Log, TEXT("Chunk Service Subsystem Initialized."));
	
}

void UChunkServiceSubsystem::UpdateChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& Voxels)
{
	if (!IsValid(GraphQLService))
	{
		UE_LOG(LogChunkService, Warning, TEXT("GQL is not valid."));
		return;
	}

	// Base64 encode the voxels
	FString EncodedVoxels = FBase64::Encode(Voxels);

	TMap<FString, FString> QueryVariables;

	QueryVariables.Add(TEXT("mapId"), FString::Printf(TEXT("%lld"), GameSessionSubsystem->GetMapID()));
	QueryVariables.Add(TEXT("x"), FString::Printf(TEXT("%lld"), X));
	QueryVariables.Add(TEXT("y"), FString::Printf(TEXT("%lld"), Y));
	QueryVariables.Add(TEXT("z"), FString::Printf(TEXT("%lld"), Z));
	QueryVariables.Add(TEXT("voxels"), EncodedVoxels);

	GraphQLService->ExecuteQueryByID(EGraphQLQuery::UpdateChunk, QueryVariables);
}

void UChunkServiceSubsystem::GetChunkByDistance(const int64 X, const int64 Y, const int64 Z, const int32 MaxDistance, const int32 Limit, const int32 Skip)
{
	if (!IsValid(GraphQLService))
	{
		UE_LOG(LogChunkService, Warning, TEXT("GQL is not valid"));
		return;
	}

	TMap<FString, FString> QueryVariables;

	// Use the input object structure with dot notation
	QueryVariables.Add(TEXT("input.mapId"), FString::Printf(TEXT("%lld"), GameSessionSubsystem->GetMapID()));
	QueryVariables.Add(TEXT("input.centerCoordinate.x"), FString::Printf(TEXT("%lld"), X));
	QueryVariables.Add(TEXT("input.centerCoordinate.y"), FString::Printf(TEXT("%lld"), Y));
	QueryVariables.Add(TEXT("input.centerCoordinate.z"), FString::Printf(TEXT("%lld"), Z));
	QueryVariables.Add(TEXT("input.maxDistance"), FString::Printf(TEXT("%d"), MaxDistance));
	QueryVariables.Add(TEXT("input.limit"), FString::Printf(TEXT("%d"), Limit));
	QueryVariables.Add(TEXT("input.skip"), FString::Printf(TEXT("%d"), Skip));

	GraphQLService->ExecuteQueryByID(EGraphQLQuery::GetChunkByDistance, QueryVariables, true, true);
	
}

void UChunkServiceSubsystem::HandleUpdateChunkResponse(const TSharedPtr<FJsonObject>& Payload) const
{
	if (!Payload.IsValid())
	{
		UE_LOG(LogChunkService, Warning, TEXT("Invalid payload."));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
		{
			OnChunkUpdated.Broadcast(false, 0, 0, 0);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		return;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* UpdateChunkObject;

	// Extract data object first
	if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogChunkService, Error, TEXT("Failed to extract data object from response"));
		return;
	}

	// Extract updateChunk object from data
	if (!(*DataObject)->TryGetObjectField(TEXT("updateChunk"), UpdateChunkObject) || !UpdateChunkObject->IsValid())
	{
		UE_LOG(LogChunkService, Error, TEXT("Failed to extract updateChunk object from data"));
		return;
	}

	// Extract coordinates from updateChunk object
	int64 X, Y, Z;
	if (!UFL_Serialization::ExtractChunkCoordinates(*UpdateChunkObject, X, Y, Z))
	{
		UE_LOG(LogChunkService, Error, TEXT("Failed to extract chunk coordinates from response json"));
		return;
	}

	// Dispatch the call for Update Chunk to Game Thread.
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, X, Y, Z]
	{
		OnChunkUpdated.Broadcast(true, X, Y, Z);
	}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}

void UChunkServiceSubsystem::HandleGetChunkResponse(const TSharedPtr<FJsonObject>& Payload) const
 {
 	if (!Payload.IsValid())
 	{
 		UE_LOG(LogChunkService, Warning, TEXT("Invalid payload for GetChunksByDistance."));
 		return;
 	}
 
 	const TSharedPtr<FJsonObject>* DataObject;
 	const TSharedPtr<FJsonObject>* GetChunksByDistanceObject;
 
 	// Extract data object first
 	if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
 	{
 		UE_LOG(LogChunkService, Error, TEXT("Failed to extract data object from GetChunksByDistance response"));
 		return;
 	}
 
 	// Extract getChunksByDistance object from data
 	if (!(*DataObject)->TryGetObjectField(TEXT("getChunksByDistance"), GetChunksByDistanceObject) || !GetChunksByDistanceObject->IsValid())
 	{
 		UE_LOG(LogChunkService, Error, TEXT("Failed to extract getChunksByDistance object from data"));
 		return;
 	}
 
 	// Extract chunks array
 	const TArray<TSharedPtr<FJsonValue>>* ChunksArray;
 	if (!(*GetChunksByDistanceObject)->TryGetArrayField(TEXT("chunks"), ChunksArray))
 	{
 		UE_LOG(LogChunkService, Error, TEXT("Failed to extract chunks array from response"));
 		return;
 	}
	
	TArray<FChunkDataContainer> AllChunkData;
	
 	// Process each chunk
 	for (const auto& ChunkValue : *ChunksArray)
 	{
 		const TSharedPtr<FJsonObject>* ChunkObject;
 		if (!ChunkValue->TryGetObject(ChunkObject))
 		{
 			UE_LOG(LogChunkService, Warning, TEXT("Failed to extract chunk object"));
 			continue;
 		}
 
 		// Extract coordinates object
 		const TSharedPtr<FJsonObject>* CoordinatesObject;
 		if (!(*ChunkObject)->TryGetObjectField(TEXT("coordinates"), CoordinatesObject))
 		{
 			UE_LOG(LogChunkService, Warning, TEXT("Failed to extract coordinates object from chunk"));
 			continue;
 		}
 
 		// Extract coordinate strings and convert to int64
 		FString sX, sY, sZ;
 		if (!(*CoordinatesObject)->TryGetStringField(TEXT("x"), sX) ||
 			!(*CoordinatesObject)->TryGetStringField(TEXT("y"), sY) ||
 			!(*CoordinatesObject)->TryGetStringField(TEXT("z"), sZ))
 		{
 			UE_LOG(LogChunkService, Warning, TEXT("Failed to extract coordinate strings from chunk"));
 			continue;
 		}
 
 		int64 X = FCString::Atoi64(*sX);
 		int64 Y = FCString::Atoi64(*sY);
 		int64 Z = FCString::Atoi64(*sZ);

 		FChunkDataContainer ChunkData;
 		ChunkData.ChunkCoordinate = FInt64Vector(X, Y, Z);
		
 		// Extract and decode voxels
 		FString EncodedVoxels;
 		if (!(*ChunkObject)->TryGetStringField(TEXT("voxels"), EncodedVoxels))
 		{
 			UE_LOG(LogChunkService, Warning, TEXT("Failed to extract encoded voxels for chunk (%lld, %lld, %lld)"), X, Y, Z);
 			continue;
 		}
 
 		TArray<uint8> Voxels;
 		Voxels.SetNumUninitialized(16 * 16 * 16);
 
 		if (!FBase64::Decode(EncodedVoxels, Voxels))
 		{
 			UE_LOG(LogChunkService, Warning, TEXT("Failed to decode voxel data for chunk (%lld, %lld, %lld)"), X, Y, Z);
 			continue;
 		}

		ChunkData.VoxelData = Voxels;
 		
 		// Extract VoxelStates
	    const TArray<TSharedPtr<FJsonValue>>* VoxelStatesArray;
		
 		if ((*ChunkObject)->TryGetArrayField(TEXT("voxelStates"), VoxelStatesArray))
	    {
		    TArray<FChunkVoxelState> VoxelStates;
		    for (const auto& VoxelStateVal : *VoxelStatesArray)
 			{
 				const TSharedPtr<FJsonObject>* VoxelStateObj;
 				if (!VoxelStateVal->TryGetObject(VoxelStateObj))
 				{
 					UE_LOG(LogChunkService, Warning, TEXT("Failed to extract voxel state object"));
 					continue;
 				}
 
 				const TSharedPtr<FJsonObject>* VoxelCoordObject;
 				if (!(*VoxelStateObj)->TryGetObjectField(TEXT("voxelCoord"), VoxelCoordObject))
 				{
 					UE_LOG(LogChunkService, Warning, TEXT("Coordinates not found in voxel state object"));
 					continue;
 				}
 
 				FString sVx, sVy, sVz, sType, sState;

		    	if (!(*VoxelCoordObject)->TryGetStringField(TEXT("x"), sVx) ||
 					!(*VoxelCoordObject)->TryGetStringField(TEXT("y"), sVy) ||
 					!(*VoxelCoordObject)->TryGetStringField(TEXT("z"), sVz) ||
 					!(*VoxelStateObj)->TryGetStringField(TEXT("voxelType"), sType) ||
 					!(*VoxelStateObj)->TryGetStringField(TEXT("state"), sState))
 				{
 					UE_LOG(LogChunkService, Warning, TEXT("Failed to extract voxel state fields"));
 					continue;
 				}
 
 				uint8 Vx = static_cast<uint8>(FCString::Atoi(*sVx));
 				uint8 Vy = static_cast<uint8>(FCString::Atoi(*sVy));
 				uint8 Vz = static_cast<uint8>(FCString::Atoi(*sVz));
 				uint8 VoxelType = static_cast<uint8>(FCString::Atoi(*sType));
 
 				TArray<uint8> DecodedState;
 				if (!FBase64::Decode(sState, DecodedState))
 				{
 					UE_LOG(LogChunkService, Warning, TEXT("Failed to decode voxel state"));
 					continue;
 				}
 
 				FVoxelState VoxelState;
                VoxelState.DeserializeFromBytes(DecodedState);

		    	ChunkData.VoxelStatesMap.Add(FVoxelCoordinate(Vx, Vy, Vz), FVoxelDefinition(1, VoxelType, VoxelState));
		    	UE_LOG(LogTemp, Log, TEXT("ChunkDataVoxelStatesMapNum:%d"), ChunkData.VoxelStatesMap.Num());
 			}
 		}

 		UE_LOG(LogTemp, Log, TEXT("ChunkService:: Voxel states not found for this chunk"));
 		AllChunkData.Add(ChunkData);
 	}

	UE_LOG(LogChunkService, Log, TEXT("Processing %d chunks from GetChunksByDistance response"), AllChunkData.Num());
	ChunkDataManager->OnGetChunkDataReceived(true, AllChunkData);
 }
