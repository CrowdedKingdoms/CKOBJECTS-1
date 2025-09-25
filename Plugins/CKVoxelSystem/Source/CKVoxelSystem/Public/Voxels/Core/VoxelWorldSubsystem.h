// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include <unordered_map>
#include <utility>
#include <tuple>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VoxelChunk.h"
#include "Shared/Types/Structures/Voxels/FVoxelListItem.h"
#include "Shared/Types/Structures/Voxels/FChunkData.h"
#include "Shared/Types/Core/Common.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "Shared/Types/Structures/Chunks/FChunkDataContainer.h"
#include "Shared/Types/Structures/Voxels/FOriginOffset.h"
#include "Shared/Types/Structures/Voxels/FChunkVoxelState.h"
#include "VoxelWorldSubsystem.generated.h"

// Struct for exporting OriginOffset to BPs as FInt64Vector is not supported. 

class UChunkServiceSubsystem;

USTRUCT(Blueprintable, BlueprintType)
struct FCurrentLocation
{
	GENERATED_BODY()
	
public:

	UPROPERTY(Blueprintable,BlueprintReadWrite, Category="Current Location")
	int64 ChunkX;

	UPROPERTY(Blueprintable,BlueprintReadWrite, Category="Current Location")
	int64 ChunkY;

	UPROPERTY(Blueprintable, BlueprintReadWrite, Category="Current Location")
	int64 ChunkZ;

	UPROPERTY(Blueprintable, BlueprintReadWrite, Category="Current Location")
	int VoxelX;

	UPROPERTY(Blueprintable, BlueprintReadWrite, Category="Current Location")
	int VoxelY;

	UPROPERTY(Blueprintable, BlueprintReadWrite, Category="Current Location")
	int VoxelZ;
};

DECLARE_LOG_CATEGORY_EXTERN(LogVoxel, Log, All);

// Delegate for loading map section based on two corner voxel chunks
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnLoadSection, int64, X0, int64, Y0, int64, Z0, int64, X1, int64, Y1, int64, Z1);

// Delegate for OnOrginRebase
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOriginRebased);

/**
 * Class that manages voxel chunks of the world
 */
UCLASS(Blueprintable, BlueprintType)
class   UVoxelWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void SetReferences(APawn* PawnReference, UTexture* DefaultAtlas);
	
	// Load/Unload voxel chunks based on the current player position.
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void UpdateWorld(FVector PlayerWorldLocation);
	
	// Does nothing if the target chunk is within a distance threshold, otherwise adjust OriginOffset and reload chunks
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void TeleportToChunk(int64 X, int64 Y, int64 Z);

	// Returns true if given chunk coordinates are within safe range for teleport, returns false if offset needs to be adjusted
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	bool ChunkCoordsInRange(int64 X, int64 Y, int64 Z) const;

	// Returns chunk origin position in unreal coordinates, taking world origin offset into account
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	FVector CalculateChunkWorldPositionOrigin(int64 X, int64 Y, int64 Z);

	// Outputs X,Y,Z of the chunk at worldLocation
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void CalculateChunkCoordinatesAtWorldLocation(FVector worldLocation, int64& X, int64& Y, int64& Z) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void SetLoadDistance(int distance);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void SetRenderDistance(int Distance);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	AVoxelChunk* UpdateVoxels(int64 X, int64 Y, int64 Z, const TArray<FVoxelListItem>& voxels);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void UpdateTextureAtlas(UTexture* TextureAtlas);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	AVoxelChunk* UpdateChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& voxels, const TArray<FChunkVoxelState>& States);

	UFUNCTION()
	void UpdateChunks(const TArray<FChunkDataContainer>& NewChunksData);

	UFUNCTION()
	void ApplyVoxelUpdatesOnChunks(const TArray<FChunkDataContainer>& VoxelUpdateData);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void UnloadFarChunks();

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void UnloadAllChunks();

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	AVoxelChunk* CreateVoxelChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& voxels, const TArray<FChunkVoxelState>& States);

	AVoxelChunk* CreateVoxelChunk(int64 X, int64 Y, int64 Z, const TArray<uint8>& voxels);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	AVoxelChunk* CreateVoxelCube(int64 X, int64 Y, int64 Z, const uint8 voxel);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	AVoxelChunk* GetChunk(int64 X, int64 Y, int64 Z);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	AVoxelChunk* GetChunkAtWorldCoord(FVector worldLocation);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	FCurrentLocation GetLocationString(FVector worldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void SetCurrentChunk(AVoxelChunk* chunk);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void UpdateVoxelAtWorldCoord(FVector worldLocation, const uint8 voxel);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	const TArray<uint8> GetFullArray(uint8 const voxel);

	//Perlin Noise Mesh Generation
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	TArray<uint8> GenerateVoxelsWithPerlinNoise(int64 Xmin, int64 Ymin, int64 Zmin, int64 Xmax, int64 Ymax, int64 Zmax, const uint8 voxelType, const float frequencyInput, const float amplitudeInput, const float thresholdInput, int
	                                            seedValue);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	TArray<FChunkData> PopulateChunksFromPerlinNoise(TArray<uint8> GeneratedVoxels, int64 Xmin, int64 Ymin, int64 Zmin, int64 Xmax, int64 Ymax, int64 Zmax);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	TArray<FChunkData> CalculateAllChunkCoordinates(int64 Xmin, int64 Ymin, int64 Zmin, int64 Xmax, int64 Ymax, int64 Zmax);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void ToggleChunkGridLines(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void ToggleChunkBillboards(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void ToggleVoxelBillboards(bool bEnable);
	
	UFUNCTION(BlueprintCallable, Category="Voxel World Controller")
	bool ValidateOriginChunks();

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void SetAtlasManager(AAtlasManager* InAtlasManager){AtlasManager = InAtlasManager;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Voxel World Controller")
	AAtlasManager* GetAtlasManager() const {return AtlasManager;}
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Voxel World Controller")
	FOriginOffset GetOriginOffset() const
	{
		FOriginOffset OrOffset;
		OrOffset.OffsetX = OriginOffset.X * CHUNK_SIZE_UNREAL;
		OrOffset.OffsetY = OriginOffset.Y * CHUNK_SIZE_UNREAL;
		OrOffset.OffsetZ = OriginOffset.Z * CHUNK_SIZE_UNREAL;
		return OrOffset;
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel World Controller")
	void SetVLOMeshProvider(AVLOMeshProvider* InVLOMeshProvider);
	
	
	//Perlin Noise Helper Functions
	static float PerlinNoise(const float X, const float Y);
	static float PerlinNoise(const float X, const float Y, const float Z);

	// Square of distance from origin at which origin rebasing is done
	static constexpr double REBASE_THRESHOLD { FMath::Square(10 * 1000 * 100.0) }; // 10 kilometers squared

	// REBASE_THRESHOLD adjusted for chunk coordinates
	static constexpr double CHUNK_THRESHOLD  { REBASE_THRESHOLD / FMath::Square(CHUNK_SIZE_UNREAL) };

	static constexpr double OCCLUSION_THRESHOLD { 0.9 };

	UPROPERTY(BlueprintAssignable, Category = "Voxel World Controller")
	FOnLoadSection OnLoadSection;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	TObjectPtr<UTexture> CurrentTextureAtlas;

	UPROPERTY(BlueprintAssignable, Category = "Voxel World Controller")
	FOnOriginRebased OnOriginRebased;
	

private:
	
	// Map of voxel chunks
	UPROPERTY()
	TMap<FChunkCoordinate, AVoxelChunk*> VoxelChunks;

	/*
		View/Render distance in chunks.
		Chunks beyond this distance will not be rendered on screen.
	*/
	UPROPERTY()
	int RenderDistance = 20;

	/*
		Load distance in chunks.
		Chunks beyond this distance will be deleted from memory.
	*/
	UPROPERTY()
	int LoadDistance = 20;

	// Chunk that the player is currently in
	UPROPERTY()
	AVoxelChunk* CurrentChunk = nullptr;

	UPROPERTY()
	FInt64Vector CurrentChunkCoord { 0, 0, 0 };

	UPROPERTY()
	bool bVoxelBillboardsEnabled = false;

	// Current unreal origin offset
	UPROPERTY()
	FInt64Vector OriginOffset { 0, 0, 0 };

	// Queue for chunks to be loaded
	UPROPERTY()
	TArray<FInt64Vector> ChunksToLoad;
	
	UPROPERTY()
	APawn* Player = nullptr;

	UPROPERTY()
	AAtlasManager* AtlasManager = nullptr;

	UPROPERTY()
	AVLOMeshProvider* VLOMeshProvider = nullptr;

	UFUNCTION()
	void RequestChunksAround(const FInt64Vector& CenterChunk, int32 Radius) const;

	UPROPERTY()
	UChunkServiceSubsystem* ChunkServiceSubsystem;

	FCriticalSection ChunksLock;
	
	UPROPERTY()
	FInt64Vector LastRequestedCenterChunk = FInt64Vector(0, 0, 0);
	
	
};
