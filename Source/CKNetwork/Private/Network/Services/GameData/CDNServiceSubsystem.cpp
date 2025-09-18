// Fill out your copyright notice in the Description page of Project Settings.

#include "CKNetwork/Pubilc/Network/Services/GameData/CDNServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Infrastructure/NetworkMessageParser.h"
#include "HttpModule.h"
#include "CKNetwork/Pubilc/FunctionLibraries/Network/FL_Serialization.h"
#include "Interfaces/IHttpResponse.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"
#include "CKVoxelSystem/Public/Voxels/Data/VoxelDataManager.h"
#include "CKVoxelSystem/Public/Voxels/Rendering/ChunkDataManager.h"
#include "Shared/Types/Structures/Voxels/FChunkVoxelState.h"


DEFINE_LOG_CATEGORY(LogCDNService);

void UCDNServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CDNStats.CDNRequestsPerSecond = 0;
	CDNStats.CDNResponsePerSecond = 0;
}

void UCDNServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UCDNServiceSubsystem::PostSubsystemInit()
{
	ChunkServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UChunkServiceSubsystem>();
	VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	VoxelDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelDataManager>();
	ChunkDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UChunkDataManager>();

	if (!ChunkServiceSubsystem || !VoxelServiceSubsystem || !GameSessionSubsystem || !VoxelDataManager || !ChunkDataManager)
	{
		UE_LOG(LogCDNService, Error, TEXT("Some or one of the subsystem is invalid."));
		return;
	}

	StartCDNStatsTimer();
	
	UE_LOG(LogCDNService, Log, TEXT("CDN Service Subsystem Initialized."));
}


bool UCDNServiceSubsystem::GetChunkCDN(int64 X, int64 Y, int64 Z)
{
	FHttpModule* Http = &FHttpModule::Get();

	// Create an HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

	// Set the URL for the request
	FString URL = FString::Printf(TEXT("%s/m/0/%lld/0/%lld/0/%lld/0/%lld/0/d2.bin"), 
		*CDN_Endpoint, GameSessionSubsystem->GetMapID(), X, Y, Z);
	Request->SetURL(URL);

	// Bind a callback function that handles the response
	Request->OnProcessRequestComplete().BindUObject(this, &UCDNServiceSubsystem::OnCDNResponseReceived);

	// Send the request
	if (Request->ProcessRequest())
	{
		//UE_LOG(LogCDNService, Log, TEXT("GET_CHUNK_CDN sent: %lld %lld %lld"), X, Y, Z);

		// Stats
		CDNRequestsPerSecond.Increment();
		int32 PayloadSize = 0;
		const FString Method = Request->GetVerb();
		PayloadSize = Method.Len();
		TArray<FString> Headers = Request->GetAllHeaders();
		for (const FString& Header : Headers)
		{
			PayloadSize += Header.Len();
		}
		PayloadSize += Request->GetContentLength();

		CDNOutgoingBytes.Set(CDNOutgoingBytes.GetValue() + PayloadSize);


		return true;
	}

	UE_LOG(LogCDNService, Warning, TEXT("GET_CHUNK_CDN not sent:  %lld %lld %lld"), X, Y, Z);
	return false;
}



FCDNStats UCDNServiceSubsystem::GetCDNStats() const
{
	CDNStats.CDNRequestsPerSecond = CDNRequestsPerSecond.GetValue();
	CDNStats.CDNResponsePerSecond = CDNResponsePerSecond.GetValue();
	CDNStats.CDNOutgoingBytes = CDNOutgoingBytes.GetValue();
	CDNStats.CDNIncomingBytes = CDNIncomingBytes.GetValue();
	return CDNStats;
}

void UCDNServiceSubsystem::StartCDNStatsTimer()
{
	AsyncTask(
		ENamedThreads::GameThread,
		[this]()
		{
			GetWorld()->GetTimerManager().SetTimer(CDNStatsTimerHandle, this, &UCDNServiceSubsystem::UpdateCDNStats, 1.0f, true);
		});
}

void UCDNServiceSubsystem::UpdateCDNStats()
{
	CDNRequestsPerSecond.Set(0);
	CDNResponsePerSecond.Set(0);
	CDNOutgoingBytes.Set(0);
	CDNIncomingBytes.Set(0);
}

void UCDNServiceSubsystem::OnCDNResponseReceived(const FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogCDNService, Error, TEXT("CDN HTTP request failed: %s"), *Request->GetURL());
		return;
	}

	if (!Response.IsValid())
	{
		UE_LOG(LogCDNService, Error, TEXT("CDN HTTP response invalid"));
		return;
	}

	// Extract chunk coords from request url
	FString ReqUrl = Request->GetURL();
	TArray<FString> URLTokens;
	ReqUrl.ParseIntoArray(URLTokens, TEXT("/0/"), true);

	if (URLTokens.Num() < 4)
	{
		UE_LOG(LogCDNService, Error, TEXT("Unable to parse chunk coordinates"));
		return;
	}

	// Stats
	CDNResponsePerSecond.Increment();

	const int64 X = FCString::Atoi(*URLTokens[2]);
	const int64 Y = FCString::Atoi(*URLTokens[3]);
	const int64 Z = FCString::Atoi(*URLTokens[4]);

	UE_LOG(LogCDNService, Log, TEXT("OnCDNResponseReceived: Received chunk data for %lld, %lld, %lld"), X, Y, Z);
	
	// Get chunk voxel data 
	TArray<uint8> RespContent = Response->GetContent();
	
	if (RespContent.Num() < 4096)
	{
		UE_LOG(LogCDNService, Warning, TEXT("OnCDNResponseReceived: Payload does not contain enough data for voxel data: %d"), RespContent.Num());
		FInt64Vector ChunkCoord = FInt64Vector(X, Y, Z);
		FChunkDataContainer DataContainer;
		ChunkDataManager->OnCDNDataReceived(false, ChunkCoord, DataContainer);
		return;
	}

	int32 Offset = 0;

	TArray<uint8> Voxels;
	Voxels.SetNumUninitialized(16*16*16);
	FMemory::Memcpy(Voxels.GetData(), RespContent.GetData() + Offset, Voxels.Num());
	Offset += 4096;

	uint16 VoxelStatesLength;
	UFL_Serialization::DeserializeValue(RespContent, VoxelStatesLength, Offset);
	Offset += sizeof(VoxelStatesLength);

	UE_LOG(LogVoxelService, Log, TEXT("OnCDNResponseReceived: Voxel States Length: %d"), VoxelStatesLength);

	TArray<FChunkVoxelState> VoxelStates;
	const int32 StartOffset = Offset;
	const int32 EndOffset = StartOffset + VoxelStatesLength;

	if (EndOffset > RespContent.Num())
	{
		//UE_LOG(LogCDNService, Error, TEXT("OnCDNResponseReceived: Payload does not contain enough data for states"));
		FChunkDataContainer DataContainer;
		DataContainer.VoxelData = Voxels;
		ChunkDataManager->OnCDNDataReceived(true, FInt64Vector(X, Y, Z), DataContainer);
		//VoxelDataManager->OnCDNResponseReceived(false, X, Y, Z, {}, {}, PreviousTimestamp);
		return;
	}

	//UE_LOG(LogCDNService, Log, TEXT("OnCDNResponseReceived: StartOffset = %d, EndOffset = %d"), StartOffset, EndOffset);

	FChunkDataContainer DataContainer;
	
	while (Offset < EndOffset)
	{
		if (Offset + 4 > RespContent.Num())
		{
			UE_LOG(LogCDNService, Error, TEXT("OnCDNResponseReceived: Not enough data for voxel state header"));
			break;
		}

		uint8 x, y, z, VoxelType;
		UFL_Serialization::DeserializeValue(RespContent, x, Offset);
		Offset += sizeof(x);
		UFL_Serialization::DeserializeValue(RespContent, y, Offset);
		Offset += sizeof(y);
		UFL_Serialization::DeserializeValue(RespContent, z, Offset);
		Offset += sizeof(z);
		UFL_Serialization::DeserializeValue(RespContent, VoxelType, Offset);
		Offset += sizeof(VoxelType);

		uint16 VoxelStateLength;
		UFL_Serialization::DeserializeValue(RespContent, VoxelStateLength, Offset);
		Offset += sizeof(VoxelStateLength);

		//UE_LOG(LogCDNService, Log, TEXT("OnCDNResponseReceived: Each Voxel State Length:%d, expected size:%llu"), VoxelStateLength, sizeof(FVoxelState));

		FChunkVoxelState ChunkVoxelState;
		ChunkVoxelState.Vx = x;
		ChunkVoxelState.Vy = y;
		ChunkVoxelState.Vz = z;
		ChunkVoxelState.VoxelType = VoxelType;

		//UE_LOG(LogCDNService, Log, TEXT("OnCDNResponseReceived: All Voxel Data: Coords: %d, %d, %d, Type: %d"), ChunkVoxelState.Vx,
		//		   ChunkVoxelState.Vy, ChunkVoxelState.Vz, ChunkVoxelState.VoxelType);

		if (VoxelStateLength > 0)
		{
			FVoxelState VoxelState;

			if (RespContent[Offset] == 1)
			{
				VoxelState.DeserializeFromBytes(RespContent, Offset);
			}
			
			ChunkVoxelState.VoxelState = VoxelState;
			Offset += VoxelStateLength;
			VoxelStates.Add(ChunkVoxelState);
			DataContainer.VoxelStatesMap.Add(FVoxelCoordinate(x, y,z), FVoxelDefinition(1, VoxelType, VoxelState));
		}
	}

	
	// Extract the 'Last-Modified' header
	FString LastModified = Response->GetHeader("Last-Modified");
	FDateTime DateTime;

	UE_LOG(LogCDNService, Log, TEXT("Last-Modified: %s, Chunk Data:%d,  Chunk Coordinates %lld, %lld, %lld"), *LastModified, RespContent.Num(), X, Y, Z);
	
	
	if (FDateTime::ParseHttpDate(LastModified, DateTime) && RespContent.Num() >= 4096)
	{
		int32 UnixTimestamp = static_cast<int32>(DateTime.ToUnixTimestamp());
		PreviousTimestamp = UnixTimestamp;
		DataContainer.VoxelData = Voxels;
		ChunkDataManager->OnCDNDataReceived(true, FInt64Vector(X,Y, Z), DataContainer);
	}
	else
	{
		ChunkDataManager->OnCDNDataReceived(true, FInt64Vector(X, Y, Z), DataContainer);
	}
	
}
