// Fill out your copyright notice in the Description page of Project Settings.

#include "Voxels/Core/VoxelWorldSubsystem.h"
#include "ProceduralMeshComponent.h"
#include "DSP/MidiNoteQuantizer.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Shared/Types/Core/Common.h"
#include <limits>
#include "CKNetwork/Pubilc/Network/Services/GameData/ChunkServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Data/VoxelDataManager.h"
#include "Voxels/Rendering/ChunkDataManager.h"
#include "Voxels/Rendering/VLOMeshProvider.h"

DEFINE_LOG_CATEGORY(LogVoxel);

void UVoxelWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UVoxelWorldSubsystem::Deinitialize()
{
	VoxelChunks.Empty();
	Super::Deinitialize();
}

void UVoxelWorldSubsystem::SetReferences(APawn* PawnReference, UTexture* DefaultAtlas = nullptr)
{
	this->CurrentTextureAtlas = DefaultAtlas;
	this->Player = PawnReference;
}


void UVoxelWorldSubsystem::UpdateWorld(const FVector PlayerWorldLocation)
{
	const int64 XU = FMath::FloorToInt(PlayerWorldLocation.X / CHUNK_SIZE_UNREAL);
	const int64 YU = FMath::FloorToInt(PlayerWorldLocation.Y / CHUNK_SIZE_UNREAL);
	const int64 ZU = FMath::FloorToInt(PlayerWorldLocation.Z / CHUNK_SIZE_UNREAL);

	const int64 X = XU + OriginOffset.X;
	const int64 Y = YU + OriginOffset.Y;
	const int64 Z = ZU + OriginOffset.Z;

	const FInt64Vector CurrentPlayerChunkCoordinate = {X, Y, Z};

	//UnloadFarChunks();

	if (CurrentChunkCoord == CurrentPlayerChunkCoordinate)
	{
		// Update only when the player moves to a different chunk
		return;
	}
	
	// Check if world origin rebasing is needed
	if (PlayerWorldLocation.SizeSquared() >= REBASE_THRESHOLD)
	{
		if (GetWorld()->SetNewWorldOrigin(FIntVector(XU * CHUNK_SIZE_UNREAL, YU * CHUNK_SIZE_UNREAL,
		                                             ZU * CHUNK_SIZE_UNREAL)))
		{
			OriginOffset.X += XU;
			OriginOffset.Y += YU;
			OriginOffset.Z += ZU;

			//UE_LOG(LogVoxel, Log, TEXT("Rebase Origin to: %lld, %lld, %lld Current offset: %lld, %lld, %lld"),
			//    XU, YU, ZU, OriginOffset.X, OriginOffset.Y, OriginOffset.Z);

			// To let BPs know origin has been rebased.
			OnOriginRebased.Broadcast();
		}
		else
		{
			UE_LOG(LogVoxel, Warning,
			       TEXT("Failed to update world origin: %lld, %lld, %lld Current offset: %lld, %lld, %lld"),
			       XU, YU, ZU, OriginOffset.X, OriginOffset.Y, OriginOffset.Z);
		}
	}
	

	const int32 DesiredRadius = LoadDistance;

	const bool bOutsideLoadedRange = FMath::Abs(CurrentPlayerChunkCoordinate.X - LastRequestedCenterChunk.X) > 3 ||
		FMath::Abs(CurrentPlayerChunkCoordinate.Y - LastRequestedCenterChunk.Y) > 3 ||
		FMath::Abs(CurrentPlayerChunkCoordinate.Z - LastRequestedCenterChunk.Z) > 3;

	if (bOutsideLoadedRange)
	{
		LastRequestedCenterChunk = CurrentPlayerChunkCoordinate;
		RequestChunksAround(CurrentPlayerChunkCoordinate, DesiredRadius);
	}
	
	AVoxelChunk* Chunk = GetChunkAtWorldCoord(PlayerWorldLocation);
	SetCurrentChunk(Chunk);
	CurrentChunkCoord = CurrentPlayerChunkCoordinate;
}

void UVoxelWorldSubsystem::TeleportToChunk(int64 X, int64 Y, int64 Z)
{
	//UE_LOG(LogVoxel, Log, TEXT("TeleportToChunk %lld, %lld, %lld"), X, Y, Z);

	if (ChunkCoordsInRange(X, Y, Z))
	{
		return; // Target chunk is within safe distance for teleport BP
	}

	// Teleporting to a far away chunk
	//UE_LOG(LogVoxel, Log, TEXT("TeleportToChunk Origin rebased to: %lld, %lld, %lld"), X, Y, Z);
	// Set origin offset and current chunk
	OriginOffset.X = X;
	OriginOffset.Y = Y;
	OriginOffset.Z = Z;

	CurrentChunkCoord = {X, Y, Z};

	UChunkDataManager* ChunkDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UChunkDataManager>();
	ChunkDataManager->UnloadAllChunkData();

	// Unload old chunks and load a new section around origin
	//UnloadAllChunks();

	// To get the Origin Offset in BPs
	OnOriginRebased.Broadcast();
}

bool UVoxelWorldSubsystem::ChunkCoordsInRange(int64 X, int64 Y, int64 Z) const
{
	// Check for operation overflows
	if ((OriginOffset.X > 0 && X < std::numeric_limits<int64>::min() + OriginOffset.X) ||
		(OriginOffset.X < 0 && X > std::numeric_limits<int64>::max() + OriginOffset.X) ||
		(OriginOffset.Y > 0 && Y < std::numeric_limits<int64>::min() + OriginOffset.Y) ||
		(OriginOffset.Y < 0 && Y > std::numeric_limits<int64>::max() + OriginOffset.Y) ||
		(OriginOffset.Z > 0 && Z < std::numeric_limits<int64>::min() + OriginOffset.Z) ||
		(OriginOffset.Z < 0 && Z > std::numeric_limits<int64>::max() + OriginOffset.Z))
	{
		return false;
	}

	// Safely calculate distance
	int64 dX = X - OriginOffset.X;
	int64 dY = Y - OriginOffset.Y;
	int64 dZ = Z - OriginOffset.Z;

	if ((FMath::Abs(dX) > CHUNK_THRESHOLD) ||
		(FMath::Abs(dY) > CHUNK_THRESHOLD) ||
		(FMath::Abs(dZ) > CHUNK_THRESHOLD) ||
		(dX * dX > CHUNK_THRESHOLD) ||
		(dY * dY > CHUNK_THRESHOLD) ||
		(dZ * dZ > CHUNK_THRESHOLD))
	{
		return false;
	}
	else if (dX * dX + dY * dY + dZ * dZ < CHUNK_THRESHOLD)
	{
		return true;
	}
	else
	{
		return false;
	}
}

FVector UVoxelWorldSubsystem::CalculateChunkWorldPositionOrigin(int64 X, int64 Y, int64 Z)
{
	FVector chunkOrigin
	{
		static_cast<double>(X - OriginOffset.X) * CHUNK_SIZE_UNREAL,
		static_cast<double>(Y - OriginOffset.Y) * CHUNK_SIZE_UNREAL,
		static_cast<double>(Z - OriginOffset.Z) * CHUNK_SIZE_UNREAL
	};

	return chunkOrigin;
}

void UVoxelWorldSubsystem::CalculateChunkCoordinatesAtWorldLocation(FVector worldLocation, int64& X, int64& Y,
                                                                    int64& Z) const
{
	X = FMath::FloorToInt(worldLocation.X / CHUNK_SIZE_UNREAL) + OriginOffset.X;
	Y = FMath::FloorToInt(worldLocation.Y / CHUNK_SIZE_UNREAL) + OriginOffset.Y;
	Z = FMath::FloorToInt(worldLocation.Z / CHUNK_SIZE_UNREAL) + OriginOffset.Z;
}

void UVoxelWorldSubsystem::SetLoadDistance(int distance)
{
	if (distance < RenderDistance || distance > MAX_LOAD_DISTANCE)
	{
		UE_LOG(LogVoxel, Warning, TEXT("Invalid Load distance: %d. Render distance is: %d"), distance, RenderDistance);
		return;
	}

	LoadDistance = distance;
}

void UVoxelWorldSubsystem::SetRenderDistance(const int Distance)
{
	if (Distance <= 0 || Distance > LoadDistance)
	{
		UE_LOG(LogVoxel, Warning, TEXT("Invalid Render distance: %d. Load distance is: %d"), Distance, LoadDistance);
		return;
	}

	//UE_LOG(LogVoxel, Log, TEXT("SetRenderDistance(%d) called"), distance);
	RenderDistance = Distance;
}

AVoxelChunk* UVoxelWorldSubsystem::UpdateVoxels(const int64 X, const int64 Y, const int64 Z,
                                                const TArray<FVoxelListItem>& voxels)
{
	//UE_LOG(LogVoxel, Log, TEXT("UpdateVoxels called"));
	FChunkCoordinate chunkPosition(DEFAULT_MAP_ID, X, Y, Z);

	AVoxelChunk* chunk = nullptr;

	if (VoxelChunks.Contains(chunkPosition))
	{
		chunk = VoxelChunks.FindRef(chunkPosition);
	}

	if (chunk != nullptr)
	{
		chunk->UpdateVoxels(voxels);
	}
	else
	{
		UE_LOG(LogVoxel, Error, TEXT("UpdateVoxels called on a chunks that does not exist"));
	}

	return chunk;
}

void UVoxelWorldSubsystem::UpdateTextureAtlas(UTexture* TextureAtlas)
{
	CurrentTextureAtlas = TextureAtlas;

	for (const auto& Chunk : VoxelChunks)
	{
		if (Chunk.Value != nullptr)
		{
			Chunk.Value->UpdateVoxelsAtlas(TextureAtlas);
		}
	}
}

AVoxelChunk* UVoxelWorldSubsystem::UpdateChunk(const int64 X, const int64 Y, const int64 Z, const TArray<uint8>& voxels,
                                               const TArray<FChunkVoxelState>& States)
{
	//UE_LOG(LogVoxel, Log, TEXT("UpdateChunk called"));
	FChunkCoordinate chunkPosition(DEFAULT_MAP_ID, X, Y, Z);

	AVoxelChunk* chunk = nullptr;

	if (VoxelChunks.Contains(chunkPosition))
	{
		chunk = VoxelChunks.FindRef(chunkPosition);
	}

	if (chunk == nullptr)
	{
		chunk = CreateVoxelChunk(X, Y, Z, voxels, States);
	}
	else
	{
		chunk->UpdateStates(States);
		chunk->UpdateChunk(voxels);
	}

	return chunk;
}

void UVoxelWorldSubsystem::UpdateChunks(const TArray<FChunkDataContainer>& NewChunksData)
{
	for (const auto& Chunk : NewChunksData)
	{
		const FChunkCoordinate ChunkPosition(DEFAULT_MAP_ID, Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y,
		                                     Chunk.ChunkCoordinate.Z);


		AVoxelChunk* ChunkActor;

		if (VoxelChunks.Contains(ChunkPosition))
		{
			ChunkActor = VoxelChunks.FindRef(ChunkPosition);
		}
		else
		{
			ChunkActor = CreateVoxelChunk(Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y, Chunk.ChunkCoordinate.Z,
			                              Chunk.VoxelData);
		}
		
		
		ChunkActor->UpdateChunk(Chunk.VoxelData);

		ChunkActor->UpdateStates(Chunk.VoxelStatesMap);
	}
}

void UVoxelWorldSubsystem::ApplyVoxelUpdatesOnChunks(const TArray<FChunkDataContainer>& VoxelUpdateData)
{
	for (const auto& Chunk : VoxelUpdateData)
	{
		const FChunkCoordinate ChunkPosition(DEFAULT_MAP_ID, Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y,
		                                     Chunk.ChunkCoordinate.Z);

		UE_LOG(LogTemp, Warning, TEXT("ApplyVoxelUpdatesOnChunks: %lld %lld %lld"), ChunkPosition.X, ChunkPosition.Y, ChunkPosition.Z);

		AVoxelChunk* ChunkActor;

		if (VoxelChunks.Contains(ChunkPosition))
		{
			ChunkActor = VoxelChunks.FindRef(ChunkPosition);
		}
		else
		{
			TArray<uint8> Voxels;
			Voxels.Init(0, NUM_VOXELS_IN_CHUNK);
			ChunkActor = CreateVoxelChunk(Chunk.ChunkCoordinate.X, Chunk.ChunkCoordinate.Y, Chunk.ChunkCoordinate.Z,
			                              Voxels);
		}

		ChunkActor->ApplyVoxelUpdates(Chunk);
		
	}
}

void UVoxelWorldSubsystem::UnloadFarChunks()
{
	//UE_LOG(LogVoxel, Log, TEXT("UnloadFarChunks called. Unload distance in chunks: %lld"), LoadDistance);

	TArray<FChunkCoordinate> ChunksToRemove;

	for (auto& ChunkPair : VoxelChunks)
	{
		// Unload chunk
		AVoxelChunk* chunkPtr = ChunkPair.Value;

		if (chunkPtr == nullptr)
		{
			UE_LOG(LogVoxel, Warning, TEXT("Null chunk detected in the map."));
			ChunksToRemove.Add(ChunkPair.Key);
			continue;
		}

		// Erase invalid chunks from map.
		if (!chunkPtr->IsValidLowLevel())
		{
			UE_LOG(LogVoxel, Warning, TEXT("Invalid chunk detected in the map."));
			ChunksToRemove.Add(ChunkPair.Key);
			continue;
		}

		const int Distance = chunkPtr->DistanceToChunk(CurrentChunkCoord);

		if (Distance > RenderDistance)
		{
			chunkPtr->SetChunkHidden(true);
		}
		else if (chunkPtr->IsChunkHidden())
		{
			chunkPtr->SetChunkHidden(false);
		}

		if (Distance > LoadDistance)
		{
			// Remove chunk from memory
			chunkPtr->Destroy();
			ChunksToRemove.Add(ChunkPair.Key);
		}
		else
		{
			// Chunk is stale
			if (Distance >= STALE_DISTANCE)
			{
				chunkPtr->SetStale(true);
			}
		}
	}

	// Remove chunks that were marked for removal
	UChunkDataManager* ChunkDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UChunkDataManager>();
	
	TArray<FInt64Vector> ChunksToRemoveCoords;
	
	for (const FChunkCoordinate& ChunkToRemove : ChunksToRemove)
	{
		ChunksToRemoveCoords.Add(FInt64Vector(ChunkToRemove.X, ChunkToRemove.Y, ChunkToRemove.Z));
		VoxelChunks.Remove(ChunkToRemove);
	}

	ChunkDataManager->UnloadFarOffChunksData(ChunksToRemoveCoords);
}

void UVoxelWorldSubsystem::UnloadAllChunks()
{
	//UE_LOG(LogVoxel, Log, TEXT("UnloadAllChunks called."));

	for (auto& chunk : VoxelChunks)
	{
		// Unload chunk
		AVoxelChunk* chunkPtr = chunk.Value;

		if (chunkPtr != nullptr)
		{
			chunkPtr->Destroy();
		}
	}

	VoxelChunks.Empty();
}

AVoxelChunk* UVoxelWorldSubsystem::CreateVoxelChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& voxels,
                                                    const TArray<FChunkVoxelState>& States)
{
	UWorld* World = GetWorld();

	if (nullptr == World)
	{
		UE_LOG(LogVoxel, Error, TEXT("World not found"));
		return nullptr;
	}

	// Set the spawn parameters
	FActorSpawnParameters SpawnParams;
	//SpawnParams.Owner = this;
	//SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the AVoxelChunk actor
	AVoxelChunk* NewChunk = World->SpawnActor<AVoxelChunk>(AVoxelChunk::StaticClass(), FVector::ZeroVector,
	                                                       FRotator::ZeroRotator, SpawnParams);

	if (!NewChunk)
	{
		UE_LOG(LogVoxel, Error, TEXT("CreateVoxelChunk failed to create chunk"));
		return nullptr;
	}

	// Initialize the chunk with data (if necessary)
	NewChunk->Initialize(X, Y, Z, VOXEL_SIZE, CHUNK_SIZE);
	NewChunk->UpdateVoxelsAtlas(CurrentTextureAtlas);
	NewChunk->SetAtlasManagerReference(AtlasManager);
	NewChunk->SetVLOMeshProvider(VLOMeshProvider);
	NewChunk->UpdateStates(States);
	NewChunk->UpdateChunk(voxels);
	
	constexpr double HALF_SIZE{CHUNK_SIZE_UNREAL / 2.0};
	NewChunk->SetActorLocation(FVector(
			(X - OriginOffset.X) * CHUNK_SIZE_UNREAL + HALF_SIZE,
			(Y - OriginOffset.Y) * CHUNK_SIZE_UNREAL + HALF_SIZE,
			(Z - OriginOffset.Z) * CHUNK_SIZE_UNREAL + HALF_SIZE)
	);

	FChunkCoordinate Coords(DEFAULT_MAP_ID, X, Y, Z);

	VoxelChunks.Add(Coords, NewChunk);

	//UE_LOG(LogVoxel, Log, TEXT("CreateVoxelChunk chunk created %lld %lld %lld"), X, Y, Z);
	return NewChunk;
}

AVoxelChunk* UVoxelWorldSubsystem::CreateVoxelChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& voxels)
{
	UWorld* World = GetWorld();

	if (nullptr == World)
	{
		UE_LOG(LogVoxel, Error, TEXT("World not found"));
		return nullptr;
	}

	// Set the spawn parameters
	FActorSpawnParameters SpawnParams;
	//SpawnParams.Owner = this;
	//SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the AVoxelChunk actor
	AVoxelChunk* NewChunk = World->SpawnActor<AVoxelChunk>(AVoxelChunk::StaticClass(), FVector::ZeroVector,
	                                                       FRotator::ZeroRotator, SpawnParams);

	if (!NewChunk)
	{
		UE_LOG(LogVoxel, Error, TEXT("CreateVoxelChunk failed to create chunk"));
		return nullptr;
	}

	// Initialize the chunk with data (if necessary)
	NewChunk->Initialize(X, Y, Z, VOXEL_SIZE, CHUNK_SIZE);
	NewChunk->UpdateChunk(voxels);
	NewChunk->UpdateVoxelsAtlas(CurrentTextureAtlas);
	NewChunk->SetAtlasManagerReference(AtlasManager);
	NewChunk->SetVLOMeshProvider(VLOMeshProvider);

	constexpr double HALF_SIZE{CHUNK_SIZE_UNREAL / 2.0};
	NewChunk->SetActorLocation(FVector(
			(X - OriginOffset.X) * CHUNK_SIZE_UNREAL + HALF_SIZE,
			(Y - OriginOffset.Y) * CHUNK_SIZE_UNREAL + HALF_SIZE,
			(Z - OriginOffset.Z) * CHUNK_SIZE_UNREAL + HALF_SIZE)
	);

	FChunkCoordinate Coords(DEFAULT_MAP_ID, X, Y, Z);

	VoxelChunks.Add(Coords, NewChunk);

	//UE_LOG(LogVoxel, Log, TEXT("CreateVoxelChunk chunk created %lld %lld %lld"), X, Y, Z);
	return NewChunk;
}

AVoxelChunk* UVoxelWorldSubsystem::CreateVoxelCube(int64 X, int64 Y, int64 Z, const uint8 voxel)
{
	//UE_LOG(LogVoxel, Log, TEXT("CreateVoxelCube: %d called"), voxel);

	TArray<uint8> voxels;
	voxels.Init(voxel, NUM_VOXELS_IN_CHUNK);
	return CreateVoxelChunk(X, Y, Z, voxels, {});
}

AVoxelChunk* UVoxelWorldSubsystem::GetChunk(int64 X, int64 Y, int64 Z)
{
	//UE_LOG(LogVoxel, Log, TEXT("GetChunk(%d, %d, %d) called"), X, Y, Z);
	FChunkCoordinate chunkPosition(DEFAULT_MAP_ID, X, Y, Z);

	if (VoxelChunks.Contains(chunkPosition))
	{
		return VoxelChunks.FindRef(chunkPosition);
	}

	return nullptr;
}

AVoxelChunk* UVoxelWorldSubsystem::GetChunkAtWorldCoord(FVector worldLocation)
{
	//UE_LOG(LogVoxel, Log, TEXT("GetChunkAtWorldCoord called (%.2f, %.2f, %.2f)"), worldLocation.X, worldLocation.Y, worldLocation.Z);
	int64 XU = FMath::FloorToInt(worldLocation.X / CHUNK_SIZE_UNREAL);
	int64 YU = FMath::FloorToInt(worldLocation.Y / CHUNK_SIZE_UNREAL);
	int64 ZU = FMath::FloorToInt(worldLocation.Z / CHUNK_SIZE_UNREAL);

	return GetChunk(XU + OriginOffset.X, YU + OriginOffset.Y, ZU + OriginOffset.Z);
}

FCurrentLocation UVoxelWorldSubsystem::GetLocationString(FVector worldLocation) const
{
	int64 X = FMath::FloorToInt(worldLocation.X / CHUNK_SIZE_UNREAL) + OriginOffset.X;
	int64 Y = FMath::FloorToInt(worldLocation.Y / CHUNK_SIZE_UNREAL) + OriginOffset.Y;
	int64 Z = FMath::FloorToInt(worldLocation.Z / CHUNK_SIZE_UNREAL) + OriginOffset.Z;

	// Voxel coords
	int x = FMath::FloorToInt(worldLocation.X) % CHUNK_SIZE_UNREAL;
	if (x < 0) x += CHUNK_SIZE_UNREAL;

	int y = FMath::FloorToInt(worldLocation.Y) % CHUNK_SIZE_UNREAL;
	if (y < 0) y += CHUNK_SIZE_UNREAL;

	int z = FMath::FloorToInt(worldLocation.Z) % CHUNK_SIZE_UNREAL;
	if (z < 0) z += CHUNK_SIZE_UNREAL;

	x /= VOXEL_SIZE;
	y /= VOXEL_SIZE;
	z /= VOXEL_SIZE;

	return {X, Y, Z, x, y, z};
}

void UVoxelWorldSubsystem::UpdateVoxelAtWorldCoord(FVector worldLocation, uint8 voxel)
{
	auto* chunk = GetChunkAtWorldCoord(worldLocation);

	if (chunk == nullptr)
	{
		UE_LOG(LogVoxel, Warning, TEXT("UpdateVoxelAtWorldCoord (%.2f, %.2f, %.2f) called but chunk not found "),
		       worldLocation.X, worldLocation.Y, worldLocation.Z);
	}

	int x = FMath::FloorToInt(worldLocation.X) % CHUNK_SIZE_UNREAL;
	if (x < 0) x += CHUNK_SIZE_UNREAL;

	int y = FMath::FloorToInt(worldLocation.Y) % CHUNK_SIZE_UNREAL;
	if (y < 0) y += CHUNK_SIZE_UNREAL;

	int z = FMath::FloorToInt(worldLocation.Z) % CHUNK_SIZE_UNREAL;
	if (z < 0) z += CHUNK_SIZE_UNREAL;

	chunk->UpdateVoxel(x / VOXEL_SIZE, y / VOXEL_SIZE, z / VOXEL_SIZE, voxel);
}

// Helpers
const TArray<uint8> UVoxelWorldSubsystem::GetFullArray(uint8 const voxel)
{
	TArray<uint8> voxelArr;
	voxelArr.Init(voxel, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
	return voxelArr;
}

void UVoxelWorldSubsystem::SetCurrentChunk(AVoxelChunk* chunk)
{
	if (CurrentChunk == chunk)
	{
		return;
	}

	if (bVoxelBillboardsEnabled)
	{
		if (CurrentChunk != nullptr)
		{
			CurrentChunk->RemoveVoxelBillboards();
		}

		if (chunk != nullptr)
		{
			chunk->AddVoxelBillboards();
		}
	}

	CurrentChunk = chunk;
}


// Visual Debuggers
void UVoxelWorldSubsystem::ToggleChunkGridLines(const bool bEnable)
{
	for (const auto& Chunk : VoxelChunks)
	{
		if (Chunk.Value == nullptr) continue;

		Chunk.Value->ToggleChunkBounds(bEnable);
	}
}

void UVoxelWorldSubsystem::ToggleChunkBillboards(const bool bEnable)
{
	for (const auto& Chunk : VoxelChunks)
	{
		if (Chunk.Value == nullptr) continue;

		if (bEnable)
		{
			Chunk.Value->AddChunkAddressBillboard();
		}
		else
		{
			Chunk.Value->RemoveChunkAddressBillboard();
		}
	}
}

void UVoxelWorldSubsystem::ToggleVoxelBillboards(const bool bEnable)
{
	bVoxelBillboardsEnabled = bEnable;

	if (CurrentChunk != nullptr)
	{
		if (bEnable)
		{
			CurrentChunk->AddVoxelBillboards();
		}
		else
		{
			CurrentChunk->RemoveVoxelBillboards();
		}
	}
}


void UVoxelWorldSubsystem::SetVLOMeshProvider(AVLOMeshProvider* InVLOMeshProvider)
{
	VLOMeshProvider = InVLOMeshProvider;
}

// Terrain Related --Start--
TArray<uint8> UVoxelWorldSubsystem::GenerateVoxelsWithPerlinNoise(int64 Xmin, int64 Ymin, int64 Zmin, int64 Xmax,
                                                                  int64 Ymax, int64 Zmax, const uint8 voxel,
                                                                  const float frequencyInput,
                                                                  const float amplitudeInput,
                                                                  const float thresholdInput, const
                                                                  int seedValue)
{
	int32 TotalChunkX = Xmax - Xmin + 1;
	int32 TotalChunkY = Ymax - Ymin + 1;
	int32 TotalChunkZ = Zmax - Zmin + 1;
	int RandomSeed = seedValue;
	float Frequency = frequencyInput;
	float Amplitude = amplitudeInput;
	float Threshold = thresholdInput;
	TArray<FInt64Vector> ChunkList;
	int32 TotalVoxelsX = TotalChunkX * CHUNK_SIZE;
	int32 TotalVoxelsY = TotalChunkY * CHUNK_SIZE;
	int32 TotalVoxelsZ = TotalChunkZ * CHUNK_SIZE;

	TArray<uint8> voxelArr;
	voxelArr.SetNumUninitialized(TotalVoxelsX * TotalVoxelsY * TotalVoxelsZ);

	for (int32 x = 0; x < TotalVoxelsX; x++)
	{
		for (int32 y = 0; y < TotalVoxelsY; y++)
		{
			for (int32 z = 0; z < TotalVoxelsZ; z++)
			{
				FVector VoxelPosition = FVector((x + RandomSeed * VOXEL_SIZE) / Frequency,
				                                (y + RandomSeed * VOXEL_SIZE) / Frequency,
				                                (z + RandomSeed * VOXEL_SIZE) / Frequency);
				float NoiseValue = PerlinNoise(VoxelPosition.X, VoxelPosition.Y, VoxelPosition.Z) * Amplitude;
				uint8 VoxelValue = (NoiseValue < Threshold) ? voxel : 0;
				voxelArr[x + TotalVoxelsX * (y + TotalVoxelsY * z)] = VoxelValue;
			}
		}
	}

	return voxelArr;
}

TArray<FChunkData> UVoxelWorldSubsystem::PopulateChunksFromPerlinNoise(TArray<uint8> GeneratedVoxels, int64 Xmin,
                                                                       int64 Ymin, int64 Zmin, int64 Xmax, int64 Ymax,
                                                                       int64 Zmax)
{
	const int32 VoxelPerChunk = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
	const int32 TotalChunks = GeneratedVoxels.Num() / VoxelPerChunk;

	int32 TotalChunksX = (Xmax - Xmin) + 1;
	int32 TotalChunksY = (Ymax - Ymin) + 1;
	int32 TotalChunksZ = (Zmax - Zmin) + 1;

	int32 TotalVoxelsX = TotalChunksX * CHUNK_SIZE;
	int32 TotalVoxelsY = TotalChunksY * CHUNK_SIZE;
	int32 TotalVoxelsZ = TotalChunksZ * CHUNK_SIZE;


	// UE_LOG(LogTemp, Log, TEXT("Total Voxels: %d, Total Chunks: %d"), GeneratedVoxels.Num(), TotalChunks);


	TMap<FInt64Vector, FChunkData> ChunkMap;


	// UE_LOG(LogTemp, Log, TEXT("X: %d, Y: %d, Z: %d"), TotalVoxelX, TotalVoxelY, TotalVoxelZ);

	for (int32 VoxelIndex = 0; VoxelIndex < GeneratedVoxels.Num(); VoxelIndex++)
	{
		//Calculate 3D Voxel Coords
		const int32 GlobalVoxelX = VoxelIndex % TotalVoxelsX;
		const int32 GlobalVoxelY = (VoxelIndex / TotalVoxelsX) % TotalVoxelsY;
		const int32 GlobalVoxelZ = VoxelIndex / (TotalVoxelsX * TotalVoxelsY);

		//Calculate Chunk Coordinates
		const int64 ChunkX = Xmin + GlobalVoxelX / CHUNK_SIZE;
		const int64 ChunkY = Ymin + GlobalVoxelY / CHUNK_SIZE;
		const int64 ChunkZ = Zmin + GlobalVoxelZ / CHUNK_SIZE;


		//int64 ChunkX = (VoxelX / CHUNK_SIZE) % (TotalVoxelsX/CHUNK_SIZE);
		//int64 ChunkY = (VoxelY / CHUNK_SIZE) % (TotalVoxelsY/CHUNK_SIZE);
		//int64 ChunkZ = (VoxelZ / CHUNK_SIZE) % TotalVoxelsZ;


		/*UE_LOG(LogTemp, Log, TEXT("Voxel Index: %d, Voxel X: %d, Voxel Y: %d, Voxel Z: %d"),
		       VoxelIndex, VoxelX, VoxelY, VoxelZ);
		UE_LOG(LogTemp, Log, TEXT("Calculated Chunk X: %lld, Chunk Y: %lld, Chunk Z: %lld"),
		       ChunkX, ChunkY, ChunkZ);
		*/

		// Not used in game, just for comparison purposes in a map
		FInt64Vector ChunkCoords(ChunkX, ChunkY, ChunkZ);

		//We check if the map already contains said Chunk
		if (!ChunkMap.Contains(ChunkCoords))
		{
			FChunkData NewChunkData;
			NewChunkData.ChunkX = ChunkX;
			NewChunkData.ChunkY = ChunkY;
			NewChunkData.ChunkZ = ChunkZ;
			NewChunkData.VoxelData.SetNumUninitialized(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
			ChunkMap.Add(ChunkCoords, NewChunkData);
		}


		FChunkData& ChunkData = ChunkMap[ChunkCoords];


		//Calculate Local Voxel Coords within Chunk, not sure if needed or not
		FVector LocalVoxelCoords;
		LocalVoxelCoords.X = GlobalVoxelX % CHUNK_SIZE;
		LocalVoxelCoords.Y = GlobalVoxelY % CHUNK_SIZE;
		LocalVoxelCoords.Z = GlobalVoxelZ % CHUNK_SIZE;
		uint8 VoxelValue = GeneratedVoxels[VoxelIndex];

		int32 LocalIndex = (LocalVoxelCoords.X * CHUNK_SIZE * CHUNK_SIZE) +
			(LocalVoxelCoords.Y * CHUNK_SIZE) +
			LocalVoxelCoords.Z;

		//Assigning the Voxel data
		ChunkData.VoxelData[LocalIndex] = GeneratedVoxels[VoxelIndex];

		/*
		UE_LOG(LogTemp, Log, TEXT("Voxel 1D Index:%d, Voxel Value: %u Voxel 3D Index: %f, %f, %f"),
		    VoxelIndex, VoxelValue, LocalVoxelCoords.X, LocalVoxelCoords.Y, LocalVoxelCoords.Z );
		 */
	}

	//Adding to array of ChunkData type, which contains structs for each chunk and their voxels.
	TArray<FChunkData> Chunks;
	Chunks.Empty();
	for (const auto& ChunkEntry : ChunkMap)
	{
		Chunks.Add(ChunkEntry.Value);
	}

	return Chunks;
}

TArray<FChunkData> UVoxelWorldSubsystem::CalculateAllChunkCoordinates(int64 Xmin, int64 Ymin, int64 Zmin, int64 Xmax,
                                                                      int64 Ymax, int64 Zmax)
{
	TArray<FChunkData> Chunks;

	for (int64 X = Xmin; X <= Xmax; X++)
	{
		for (int64 Y = Ymin; Y <= Ymax; Y++)
		{
			for (int64 Z = Zmin; Z <= Zmax; Z++)
			{
				FChunkData ChunkData;
				ChunkData.ChunkX = X;
				ChunkData.ChunkY = Y;
				ChunkData.ChunkZ = Z;
				Chunks.Add(ChunkData);
			}
		}
	}

	return Chunks;
}

float UVoxelWorldSubsystem::PerlinNoise(const float X, const float Y)
{
	return FMath::PerlinNoise2D(FVector2D(X, Y));
}

float UVoxelWorldSubsystem::PerlinNoise(const float X, const float Y, const float Z)
{
	return FMath::PerlinNoise3D(FVector(X, Y, Z));
}

// Terrain Related --End--

void UVoxelWorldSubsystem::RequestChunksAround(const FInt64Vector& CenterChunk, const int32 Radius) const
{
	UChunkDataManager* ChunkDataManager = GetWorld()->GetGameInstance()->GetSubsystem<UChunkDataManager>();
	
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, ChunkDataManager, CenterChunk]()
	{
		TArray<FInt64Vector> ChunksToRequest;
		for (int32 dx = -3; dx <= 3; ++dx)
		{
			for (int32 dy = -3; dy <= 3; ++dy)
			{
				for (int32 dz = -3; dz <= 3; ++dz)
				{
					FInt64Vector ChunkCoordinate = FInt64Vector(CenterChunk.X + dx, CenterChunk.Y + dy, CenterChunk.Z + dz);
					ChunksToRequest.Add(ChunkCoordinate);
				}
			}
		}

		ChunkDataManager->SetNewCenterCoordinate(CenterChunk);
		ChunkDataManager->EnqueueChunksForRequesting(ChunksToRequest);
		
	}, LowLevelTasks::ETaskPriority::BackgroundLow);
	
}

bool UVoxelWorldSubsystem::ValidateOriginChunks()
{
	bool bOriginValid = true;

	for (int64 X = 0; X <= 1; X++)
	{
		for (int64 Y = 0; Y <= 1; Y++)
		{
			if (!IsValid(GetChunk(X, Y, -1)))
			{
				bOriginValid = false;
				break;
			}
		}
		if (!bOriginValid)
		{
			break;
		}
	}

	if (bOriginValid)
	{
		return true; // Return true if all origin chunks are valid
	}

	return false;
}
