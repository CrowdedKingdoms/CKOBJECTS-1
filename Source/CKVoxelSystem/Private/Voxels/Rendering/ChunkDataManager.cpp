#include "Voxels/Rendering/ChunkDataManager.h"
#include "Async/Async.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/ChunkServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"

DEFINE_LOG_CATEGORY(LogChunkLoader);

void UChunkDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UChunkDataManager::Deinitialize()
{
	// Proper cleanup
	bShouldShutdown = true;
	bIsTicking = false;
	
	Super::Deinitialize();
}

void UChunkDataManager::PostSubsystemInit()
{
	ChunkServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UChunkServiceSubsystem>();
	VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	CDNServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCDNServiceSubsystem>();

	if (ChunkServiceSubsystem == nullptr || VoxelServiceSubsystem == nullptr || CDNServiceSubsystem == nullptr)
	{
		UE_LOG(LogChunkLoader, Error, TEXT("Subsystem(s) reference is invalid."));
	}

	ProcessDirtyChunks();
}

void UChunkDataManager::EnqueueChunksForRequesting(const TArray<FInt64Vector>& ChunkCoordinates)
{
	// Create a copy of the coordinates to sort
	TArray<FInt64Vector> SortedCoordinates = ChunkCoordinates;

	// Sort by distance to CurrentCenterChunkCoordinate (closest first)
	SortedCoordinates.Sort([this](const FInt64Vector& A, const FInt64Vector& B)
	{
		// Calculate squared distance to avoid expensive sqrt operations
		const int64 DistanceA = FMath::Square(A.X - CurrentCenterChunkCoordinate.X) +
			FMath::Square(A.Y - CurrentCenterChunkCoordinate.Y) +
			FMath::Square(A.Z - CurrentCenterChunkCoordinate.Z);

		const int64 DistanceB = FMath::Square(B.X - CurrentCenterChunkCoordinate.X) +
			FMath::Square(B.Y - CurrentCenterChunkCoordinate.Y) +
			FMath::Square(B.Z - CurrentCenterChunkCoordinate.Z);

		return DistanceA < DistanceB; // Sort in ascending order (closest first)
	});

	// Enqueue the sorted coordinates
	{
		FScopeLock Lock(&DataLock);
		for (const FInt64Vector& ChunkCoordinate : SortedCoordinates)
		{
			ChunksToRequest.Enqueue(ChunkCoordinate);
		}
	}

	DequeueChunksForRequesting();
}

void UChunkDataManager::LoadInitialChunks()
{
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
	{
		TArray<FInt64Vector> InitialChunksToRequest;
		constexpr int32 Range = 7;
		constexpr int32 HalfRange = Range / 2;

		for (int32 i = 0; i < Range * Range * Range; ++i)
		{
			const int32 dx = (i % Range) - HalfRange;
			const int32 dy = ((i / Range) % Range) - HalfRange;
			const int32 dz = (i / (Range * Range)) - HalfRange;

			FInt64Vector ChunkCoordinate = FInt64Vector(0 + dx, 0 + dy, 0 + dz);
			InitialChunksToRequest.Add(ChunkCoordinate);
		}

		SetNewCenterCoordinate(FInt64Vector(0, 0, 0));
		EnqueueChunksForRequesting(InitialChunksToRequest);
		
	}, LowLevelTasks::ETaskPriority::BackgroundLow);
}

void UChunkDataManager::UnloadFarOffChunksData(const TArray<FInt64Vector>& ChunkCoordinates)
{
	FScopeLock Lock(&DataLock);
	for (auto ChunkToRemove : ChunkCoordinates)
	{
		if (LoadedChunks.Contains(ChunkToRemove))
		{
			LoadedChunks.Remove(ChunkToRemove);
			UE_LOG(LogTemp, Log, TEXT("Removed chunk from Loaded Chunks: %lld, %lld, %lld"),ChunkToRemove.X, ChunkToRemove.Y, ChunkToRemove.Z)
		}

		if (RequestedChunks.Contains(ChunkToRemove))
		{
			RequestedChunks.Remove(ChunkToRemove);
			UE_LOG(LogTemp, Log, TEXT("Removed chunk from Requested Chunks: %lld, %lld, %lld"),ChunkToRemove.X, ChunkToRemove.Y, ChunkToRemove.Z)
		}
	}
	
}

void UChunkDataManager::UnloadAllChunkData()
{
	FScopeLock Lock(&DataLock);
	LoadedChunks.Empty();
	RequestedChunks.Empty();
	DirtyChunksQueue.Empty();
	UE_LOG(LogTemp, Log, TEXT("Unloaded all chunk data."))
}

void UChunkDataManager::DequeueChunksForRequesting()
{
	const int32 Timestamp = 0;

	{
		FInt64Vector ChunkCoordinate;
		FScopeLock Lock(&DataLock);

		//FThreadSafeBool bSentGetChunkRequest = false;
		
		while (ChunksToRequest.Dequeue(ChunkCoordinate))
		{
			if (RequestedChunks.Contains(ChunkCoordinate))
			{
				continue;
			}

			RequestedChunks.Add(ChunkCoordinate, FDateTime::UtcNow().ToUnixTimestamp());
			FChunkDataState NewChunkDataState;

			LoadedChunks.Add(ChunkCoordinate, NewChunkDataState);

			// if (CDNServiceSubsystem)
			// {
			// 	CDNServiceSubsystem->GetChunkCDN(ChunkCoordinate.X, ChunkCoordinate.Y, ChunkCoordinate.Z);
			// }

			// if (!bSentGetChunkRequest)
			// {
			// 	if (ChunkServiceSubsystem)
			// 	{
			// 		ChunkServiceSubsystem->GetChunkByDistance(CurrentCenterChunkCoordinate.X, CurrentCenterChunkCoordinate.Y,
			// 												  CurrentCenterChunkCoordinate.Z, 8, 0, 0);
			// 		bSentGetChunkRequest = true;
			// 	}
			// }

			if (VoxelServiceSubsystem)
			{
				VoxelServiceSubsystem->SendVoxelListRequest(ChunkCoordinate.X, ChunkCoordinate.Y, ChunkCoordinate.Z,
				                                            Timestamp);
			}
		}
		
	}
}

void UChunkDataManager::OnCDNDataReceived(const bool bSuccess, const FInt64Vector& ChunkCoordinate,
                                          const FChunkDataContainer& CDNData)
{
	if (!bSuccess)
	{
		UE_LOG(LogChunkLoader, Error, TEXT("CDN data request failed"));
		return;
	}

	bool bShouldMarkDirty = false;

	{
		FScopeLock Lock(&DataLock);
		if (FChunkDataState* ChunkState = LoadedChunks.Find(ChunkCoordinate))
		{
			const bool bDataChanged = HasDataChanged(ChunkState->CDNChunkData, CDNData);

			ChunkState->CDNChunkData = CDNData;
			ChunkState->bCDNResponseReceived = true;
			ChunkState->LastCDNUpdateTime = FDateTime::UtcNow().ToUnixTimestamp();
			ChunkState->bProcessed = false;

			if (bDataChanged)
			{
				ChunkState->bCDNDataDirty = true;
				bShouldMarkDirty = true;
				UE_LOG(LogChunkLoader, Log, TEXT("CDN data changed for chunk: %lld, %lld, %lld"),
				       ChunkCoordinate.X, ChunkCoordinate.Y, ChunkCoordinate.Z);
			}

			RequestedChunks.Remove(ChunkCoordinate);
		}
		else
		{
			UE_LOG(LogChunkLoader, Error, TEXT("CDN data received for unknown chunk: %lld, %lld, %lld"),
			       ChunkCoordinate.X, ChunkCoordinate.Y, ChunkCoordinate.Z);
		}
	}

	// Enqueue outside the lock to avoid deadlocks
	if (bShouldMarkDirty)
	{
		DirtyChunksQueue.Enqueue(ChunkCoordinate);
	}
}

void UChunkDataManager::OnGetChunkDataReceived(const bool bSuccess, TArray<FChunkDataContainer>& AllChunkData)
{
	if (!bSuccess)
	{
		UE_LOG(LogChunkLoader, Error, TEXT("GetChunk request failed"));
		return;
	}

	if (AllChunkData.Num() == 0)
	{
		UE_LOG(LogChunkLoader, Warning, TEXT("GetChunk request returned empty chunk data"));
		return;
	}

	TArray<FInt64Vector> ChunksToMarkDirty;

	{
		FScopeLock Lock(&DataLock);

		for (const FChunkDataContainer& ChunkData : AllChunkData)
		{
			const FInt64Vector ChunkCoordinate = ChunkData.ChunkCoordinate;

			FChunkDataState* ChunkState = LoadedChunks.Find(ChunkCoordinate);
			if (!ChunkState)
			{
				FChunkDataState NewChunkState;
				LoadedChunks.Add(ChunkCoordinate, NewChunkState);
				ChunkState = LoadedChunks.Find(ChunkCoordinate);
			}

			const bool bDataChanged = HasDataChanged(ChunkState->GetChunkData, ChunkData);

			ChunkState->GetChunkData = ChunkData;
			ChunkState->bChunkGetResponseReceived = true;
			ChunkState->LastGetChunkUpdateTime = FDateTime::UtcNow().ToUnixTimestamp();
			ChunkState->bProcessed = false;

			if (bDataChanged)
			{
				ChunkState->bGetChunkDataDirty = true;
				ChunksToMarkDirty.Add(ChunkCoordinate);
			}

			RequestedChunks.Remove(ChunkCoordinate);
		}
	}

	// Enqueue outside the lock
	for (const FInt64Vector& ChunkCoord : ChunksToMarkDirty)
	{
		DirtyChunksQueue.Enqueue(ChunkCoord);
	}
}

void UChunkDataManager::OnVoxelListDataReceived(const bool bSuccess, const FInt64Vector& ChunkCoordinate,
                                                const FChunkDataContainer& VoxelListData)
{
	if (!bSuccess)
	{
		UE_LOG(LogChunkLoader, Error, TEXT("Voxel list request failed"));
		return;
	}

	bool bShouldMarkDirty = false;

	{
		FScopeLock Lock(&DataLock);
		if (FChunkDataState* ChunkState = LoadedChunks.Find(ChunkCoordinate))
		{
			const bool bDataChanged = HasDataChanged(ChunkState->VoxelUpdates, VoxelListData);

			ChunkState->VoxelUpdates = VoxelListData;
			ChunkState->bVoxelListResponseReceived = true;
			ChunkState->LastVoxelListUpdateTime = FDateTime::UtcNow().ToUnixTimestamp();
			ChunkState->bProcessed = false;

			if (bDataChanged)
			{
				ChunkState->bVoxelListDataDirty = true;
				bShouldMarkDirty = true;
			}

			RequestedChunks.Remove(ChunkCoordinate);
		}
		else
		{
			UE_LOG(LogChunkLoader, Error, TEXT("Voxel list data received for unknown chunk: %lld, %lld, %lld"),
			       ChunkCoordinate.X, ChunkCoordinate.Y, ChunkCoordinate.Z);
		}
	}

	if (bShouldMarkDirty)
	{
		DirtyChunksQueue.Enqueue(ChunkCoordinate);
	}
}

bool UChunkDataManager::IsChunkDirty(const FInt64Vector& ChunkCoordinate) const
{
	if (const FChunkDataState* ChunkState = LoadedChunks.Find(ChunkCoordinate))
	{
		return ChunkState->bCDNDataDirty || ChunkState->bGetChunkDataDirty || ChunkState->bVoxelListDataDirty;
	}

	return false;
}

TArray<FInt64Vector> UChunkDataManager::GetDirtyChunks() const
{
	TArray<FInt64Vector> DirtyChunksList;

	for (const auto& ChunkPair : LoadedChunks)
	{
		if (IsChunkDirty(ChunkPair.Key))
		{
			DirtyChunksList.Add(ChunkPair.Key);
		}
	}

	return DirtyChunksList;
}

void UChunkDataManager::ProcessDirtyChunks()
{
	if (bIsTicking)
	{
		return; // Already running
	}

    // Use a dedicated OS thread to avoid blocking the task graph in packaged builds
    Async(EAsyncExecution::Thread, [this]()
    {
        double LastTime = FPlatformTime::Seconds();

        while (!bShouldShutdown)
        {
            bool bProcessedWork = false;
            const double CurrentTime = FPlatformTime::Seconds();
            const double DeltaTime = CurrentTime - LastTime;

            if (DeltaTime >= TickInterval)
            {
                ProcessDirtyChunksBatch();
                bProcessedWork = true;
                LastTime = CurrentTime;
            }

            // Only sleep if no work was processed
            if (!bProcessedWork)
            {
                FPlatformProcess::Sleep(0.0016f);
            }
        }

        bIsTicking = false;
    });
}

void UChunkDataManager::ProcessDirtyChunksBatch()
{
	// Collect dirty chunks from queue
	TArray<FInt64Vector> DirtyChunks;
	FInt64Vector ChunkCoord;

	while (DirtyChunksQueue.Dequeue(ChunkCoord) && DirtyChunks.Num() < MaxBatchSize)
	{
		DirtyChunks.Add(ChunkCoord);
	}

	if (DirtyChunks.Num() == 0)
	{
		return;
	}

	// Process the batch
	TArray<FChunkDataContainer> ProcessedChunks;

	{
		FScopeLock Lock(&DataLock);

		for (const FInt64Vector& ChunkCoordinate : DirtyChunks)
		{
			// Skip if already being processed
			if (CurrentlyProcessing.Contains(ChunkCoordinate))
			{
				continue;
			}

			FChunkDataState* ChunkState = LoadedChunks.Find(ChunkCoordinate);
			if (!ChunkState || ChunkState->bProcessed)
			{
				continue;
			}

			CurrentlyProcessing.Add(ChunkCoordinate);

			FChunkDataContainer FinalChunkData;

			// Apply your existing logic for combining data sources
			bool bHasGetChunkData = ChunkState->bChunkGetResponseReceived;
			bool bHasCDNData = ChunkState->bCDNResponseReceived;
			bool bHasVoxelUpdates = ChunkState->VoxelUpdates.VoxelStatesMap.Num() > 0;

			if (!bHasVoxelUpdates)
			{
				break;
			}
			// Priority: GetChunk > CDN > Empty
			/*
			if (bHasGetChunkData)
			{
				FinalChunkData.VoxelData = ChunkState->GetChunkData.VoxelData;
			}
			else if (bHasCDNData)
			{
				FinalChunkData.VoxelData = ChunkState->CDNChunkData.VoxelData;
			}
			else
			{
				constexpr int32 ChunkSize = 16 * 16 * 16;
				FinalChunkData.VoxelData.Init(0, ChunkSize);
			}
			*/

			//constexpr int32 ChunkSize = 16 * 16 * 16;
			//FinalChunkData.VoxelData.Init(0, ChunkSize);

			// Apply voxel updates
			//ApplyVoxelUpdates(FinalChunkData, ChunkState->VoxelUpdates);
			
			FinalChunkData = ChunkState->VoxelUpdates;
			FinalChunkData.ChunkCoordinate = ChunkCoordinate;

			ChunkState->bProcessed = true;
			ChunkState->bCDNDataDirty = false;
			ChunkState->bGetChunkDataDirty = false;
			ChunkState->bVoxelListDataDirty = false;

			ProcessedChunks.Add(MoveTemp(FinalChunkData));
		}
	}

	// Send to game thread if we have processed chunks
	if (ProcessedChunks.Num() > 0)
	{
		// Capture by value to avoid lifetime issues
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, ProcessedChunks = MoveTemp(ProcessedChunks)]()
		{
			if (UVoxelWorldSubsystem* VoxelWorld = GetWorld()->GetSubsystem<UVoxelWorldSubsystem>())
			{
				//VoxelWorld->UpdateChunks(ProcessedChunks);
				for (const FChunkDataContainer& ChunkData : ProcessedChunks)
				{
					UE_LOG(LogTemp, Log, TEXT("Chunk Data: %lld, %lld, %lld"), ChunkData.ChunkCoordinate.X, ChunkData.ChunkCoordinate.Y, ChunkData.ChunkCoordinate.Z)
				}
				VoxelWorld->ApplyVoxelUpdatesOnChunks(ProcessedChunks);
			}

			// Remove from a processing set
			{
				FScopeLock Lock(&DataLock);
				for (const FChunkDataContainer& Chunk : ProcessedChunks)
				{
					CurrentlyProcessing.Remove(Chunk.ChunkCoordinate);
				}
			}
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
	}
}

void UChunkDataManager::ApplyVoxelUpdates(FChunkDataContainer& ChunkData,
                                          const FChunkDataContainer& VoxelUpdates)
{
	ChunkData.VoxelStatesMap = VoxelUpdates.VoxelStatesMap;

	for (const auto& UpdatePair : VoxelUpdates.VoxelStatesMap)
	{
		const FVoxelCoordinate& VoxelCoord = UpdatePair.Key;
		const FVoxelDefinition& VoxelDef = UpdatePair.Value;

		// Validate coordinates
		if (VoxelCoord.X < 0 || VoxelCoord.X >= 16 ||
			VoxelCoord.Y < 0 || VoxelCoord.Y >= 16 ||
			VoxelCoord.Z < 0 || VoxelCoord.Z >= 16)
		{
			UE_LOG(LogChunkLoader, Warning, TEXT("Invalid voxel coordinate: %d, %d, %d"),
			       VoxelCoord.X, VoxelCoord.Y, VoxelCoord.Z);
			continue;
		}

		const int32 Index = VoxelCoord.X + VoxelCoord.Y * 16 + VoxelCoord.Z * 16 * 16;

		if (ChunkData.VoxelData.IsValidIndex(Index))
		{
			ChunkData.VoxelData[Index] = VoxelDef.VoxelType;
		}
	}
}

bool UChunkDataManager::HasDataChanged(const FChunkDataContainer& OldData, const FChunkDataContainer& NewData)
{
	if (OldData.VoxelData.Num() != NewData.VoxelData.Num())
	{
		return true;
	}

	if (OldData.VoxelData != NewData.VoxelData)
	{
		return true;
	}

	// Check if VoxelStatesMap has different number of entries
	if (OldData.VoxelStatesMap.Num() != NewData.VoxelStatesMap.Num())
	{
		return true;
	}

	// Iterate through the old map and compare with a new map
	for (const auto& OldVoxelPair : OldData.VoxelStatesMap)
	{
		const FVoxelCoordinate& VoxelCoord = OldVoxelPair.Key;
		const FVoxelDefinition& OldVoxelDef = OldVoxelPair.Value;

		// Check if this voxel coordinate exists in the new map
		const FVoxelDefinition* NewVoxelDef = NewData.VoxelStatesMap.Find(VoxelCoord);
		if (!NewVoxelDef)
		{
			// Voxel exists in old but not in new - data has changed
			return true;
		}

		// Compare the voxel definitions
		if (OldVoxelDef.Version != NewVoxelDef->Version ||
			OldVoxelDef.VoxelType != NewVoxelDef->VoxelType ||
			OldVoxelDef.VoxelState != NewVoxelDef->VoxelState)
		{
			// Voxel definition has changed
			return true;
		}
	}

	// Check if a new map has any voxels that an old map doesn't have
	for (const auto& NewVoxelPair : NewData.VoxelStatesMap)
	{
		const FVoxelCoordinate& VoxelCoord = NewVoxelPair.Key;

		if (!OldData.VoxelStatesMap.Contains(VoxelCoord))
		{
			// New voxel exists that wasn't in the old map-data has changed
			return true;
		}
	}

	return false;
}
