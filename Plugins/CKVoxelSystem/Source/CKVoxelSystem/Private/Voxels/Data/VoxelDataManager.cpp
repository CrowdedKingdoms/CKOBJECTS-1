// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Data/VoxelDataManager.h"
#include "Shared/Types/Structures/Chunks/FChunkDataContainer.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"


void UVoxelDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UVoxelDataManager::Deinitialize()
{
	bIsTicking = false;
	Super::Deinitialize();
}


void UVoxelDataManager::PostSubsystemInit()
{
	CDNServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCDNServiceSubsystem>();
	VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	ChunkServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UChunkServiceSubsystem>();

	if (!CDNServiceSubsystem || !VoxelServiceSubsystem || !ChunkServiceSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelDataManager::Some or one of the subsystem is invalid."));
		return;
	}

	// Batch Loop Processing
	ProcessBatch();
	
	UE_LOG(LogTemp, Log, TEXT("Voxel Data Manager Initialized."));
}

void UVoxelDataManager::OnGetChunkResponse(const bool Success, const TArray<FChunkDataContainer>& AllChunkData)
{
	if (!Success)
	{
		//UE_LOG(LogTemp, Log, TEXT("OnGetChunkResponse failed. returning"))
		return;
	}
	
	{
		//For List Chunks
		FScopeLock Lock(&DataLock);
		const int32 TimeStamp = FDateTime::UtcNow().ToUnixTimestamp();
		for (auto& Chunk : AllChunkData)
		{
			CDNServiceSubsystem->GetChunkCDN(Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y, Chunk.ChunkCoordinate.Z);
			VoxelServiceSubsystem->SendVoxelListRequest(Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y,
														Chunk.ChunkCoordinate.Z, TimeStamp);

			FVoxelDataState VoxelDataState;
			VoxelDataState.bChunkGetResponseReceived = true;
			VoxelDataState.GetChunkData = Chunk;

			PendingVoxelData.Add(FInt64Vector(Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y, Chunk.ChunkCoordinate.Z),
								 VoxelDataState);
		}

		//For Spatial Chunks
		for (auto& Coord : VoxelListCoordsToRequest)
		{
			if (!PendingVoxelData.Contains(Coord))
			{
				VoxelServiceSubsystem->SendVoxelListRequest(Coord.X, Coord.Y, Coord.Z, TimeStamp);
				FVoxelDataState VoxelDataState;
				PendingVoxelData.Add(Coord, VoxelDataState);
			}
		}
		VoxelListCoordsToRequest.Empty();
	}
	
}


void UVoxelDataManager::OnCDNResponseReceived(const bool Success, const int64 X, const int64 Y, const int64 Z,
                                              const TArray<uint8>& VoxelData,
                                              const TArray<FChunkVoxelState>& VoxelStates,
                                              const int32 LastModifiedTimestamp)
{
	if (!Success)
	{
		//UE_LOG(LogTemp, Log, TEXT("CDN Response failed. returning"))
		return;
	}

	{
		FScopeLock Lock(&DataLock);
		const FInt64Vector ChunkCoords(X, Y, Z);
		FChunkDataContainer ChunkData;
		ChunkData.ChunkCoordinate = ChunkCoords;
		ChunkData.VoxelData = VoxelData;

		for (const auto& VoxelState : VoxelStates)
		{
			ChunkData.VoxelStatesMap.Add(FVoxelCoordinate(VoxelState.Vx, VoxelState.Vy, VoxelState.Vz),
										 FVoxelDefinition(1, VoxelState.VoxelType, VoxelState.VoxelState));
		}

		if (FVoxelDataState* State = PendingVoxelData.Find(ChunkCoords))
		{
			State->CDNChunkData = ChunkData;
			State->bCDNResponseReceived = true;
		}
		else
		{
			FVoxelDataState NewState;
			NewState.CDNChunkData = ChunkData;
			NewState.bCDNResponseReceived = true;
			PendingVoxelData.Add(ChunkCoords, MoveTemp(NewState));
		}
	}
	
}


void UVoxelDataManager::OnVoxelListResponse(const bool Success, const int64 X, const int64 Y, const int64 Z,
                                            const TArray<FChunkVoxelState>& ChunkVoxelStates)
{
	if (!Success)
	{
		return;
	}

	{
		FScopeLock Lock(&DataLock);

		FChunkDataContainer ChunkData;
		ChunkData.ChunkCoordinate = FInt64Vector(X, Y, Z);
		ChunkData.VoxelData.SetNumZeroed(4096);

		for (auto& ChunkVoxelState : ChunkVoxelStates)
		{
			ChunkData.VoxelStatesMap.Add(FVoxelCoordinate(ChunkVoxelState.Vx, ChunkVoxelState.Vy, ChunkVoxelState.Vz),
										 FVoxelDefinition(1, ChunkVoxelState.VoxelType, ChunkVoxelState.VoxelState));
		}

		
		if (FVoxelDataState* State = PendingVoxelData.Find(ChunkData.ChunkCoordinate))
		{
			State->bVoxelListResponseReceived = true;
			State->VoxelUpdates = ChunkData;
		}
		else
		{
			FVoxelDataState NewState;
			NewState.bVoxelListResponseReceived = true;
			NewState.VoxelUpdates = ChunkData;
			PendingVoxelData.Add(ChunkData.ChunkCoordinate, MoveTemp(NewState));
		}
	}
	
}

void UVoxelDataManager::AddVoxelCoordinatesToRequestList(const TArray<FInt64Vector>& VoxelCoords)
{
	VoxelListCoordsToRequest.Append(VoxelCoords);
}

void UVoxelDataManager::ProcessBatch()
{
	if (bIsTicking)
	{
		return;
	}

	bIsTicking = true;

	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]
	{
		double LastTime = FPlatformTime::Seconds();
		double AccumulatedTime = 0.0;
		constexpr double TickInterval = 0.01;
		
		while (bIsTicking)
		{
			const double CurrentTime = FPlatformTime::Seconds();
			const double DeltaTime = CurrentTime - LastTime;
			LastTime = CurrentTime;
			AccumulatedTime += DeltaTime;

			if (AccumulatedTime >= TickInterval)
			{
				FScopeLock Lock(&DataLock);
				
				for (auto It = PendingVoxelData.CreateIterator(); It; ++It)
				{
					const FInt64Vector ChunkCoords = It.Key();
					FVoxelDataState& VoxelDataState = It.Value();

					if (VoxelDataState.bVoxelListResponseReceived && VoxelDataState.bChunkGetResponseReceived || VoxelDataState.bCDNResponseReceived)
					{
						FChunkDataContainer FinalChunk;
						if (VoxelDataState.GetChunkData.VoxelData.Num() > 0)
						{
							FinalChunk = VoxelDataState.GetChunkData;
							
						}
						else
						{
							FinalChunk = VoxelDataState.CDNChunkData;
						}

						if (VoxelDataState.VoxelUpdates.VoxelStatesMap.Num() > 0)
						{
							FinalChunk.VoxelStatesMap = VoxelDataState.VoxelUpdates.VoxelStatesMap;
							const TMap<FVoxelCoordinate, FVoxelDefinition>& Updates = VoxelDataState.VoxelUpdates.VoxelStatesMap;

							for (const TPair<FVoxelCoordinate, FVoxelDefinition>& Pair : Updates)
							{
								const FVoxelCoordinate& Coord = Pair.Key;
								const FVoxelDefinition& Def = Pair.Value;

								const int32 Index = Coord.X + Coord.Y * 16 + Coord.Z * 16 * 16;

								if (Index >= 0 && Index < FinalChunk.VoxelData.Num())
								{
									FinalChunk.VoxelData[Index] = Def.VoxelType;
								}
							}
						}

						FinalChunkDataArray.Add(MoveTemp(FinalChunk));
						It.RemoveCurrent();
					}
					else if (VoxelDataState.bVoxelListResponseReceived && !VoxelDataState.bChunkGetResponseReceived)
					{
						FChunkDataContainer FinalChunk;
						
						FinalChunk.ChunkCoordinate = ChunkCoords;
						
						FinalChunk.VoxelData.Init(0, 4096);
						
						if (VoxelDataState.VoxelUpdates.VoxelStatesMap.Num() > 0)
						{
							FinalChunk.VoxelStatesMap = VoxelDataState.VoxelUpdates.VoxelStatesMap;

							for (const TPair<FVoxelCoordinate, FVoxelDefinition>& Pair : VoxelDataState.VoxelUpdates.VoxelStatesMap)
							{
								const FVoxelCoordinate& Coord = Pair.Key;
								const FVoxelDefinition& Def = Pair.Value;

								const int32 Index = Coord.X + Coord.Y * 16 + Coord.Z * 16 * 16;
								if (Index >= 0 && Index < FinalChunk.VoxelData.Num())
								{
									FinalChunk.VoxelData[Index] = Def.VoxelType;
								}
							}
						}
						
						FinalChunkDataArray.Add(MoveTemp(FinalChunk));
						It.RemoveCurrent();
					}
				}

				if (FinalChunkDataArray.Num() >= 4)
				{
					UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
					{
						TArray<FChunkDataContainer> Batch = MoveTemp(FinalChunkDataArray);
						FinalChunkDataArray.Empty();
					    UE_LOG(LogTemp, Log, TEXT("Firing to game thread for rendering"));
						GetWorld()->GetSubsystem<UVoxelWorldSubsystem>()->UpdateChunks(Batch);
					}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
				}
				else if (FinalChunkDataArray.Num() >= 1)
				{
					UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
					{
						TArray<FChunkDataContainer> Batch = MoveTemp(FinalChunkDataArray);
						FinalChunkDataArray.Empty();
						UE_LOG(LogTemp, Log, TEXT("Firing to game thread for rendering"));
						GetWorld()->GetSubsystem<UVoxelWorldSubsystem>()->UpdateChunks(Batch);
					}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
				}
			}

			FPlatformProcess::Sleep(0.1);
		}
	});
}
