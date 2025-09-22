#pragma once

#include "CoreMinimal.h"
#include "Math/MathFwd.h"
#include "UObject/NoExportTypes.h"
#include "CKSharedTypes/Public/Shared/Types/Structures/Voxels/ChunkCoordinate.h"
#include "Voxels/Core/VoxelChunk.h"
#include <unordered_map>
#include <utility>
#include <tuple>


// Chunk map type
using FVoxelChunkMap = TMap<FChunkCoordinate, AVoxelChunk*>;


// Common global constants

// Voxel size in unreal units
static constexpr int VOXEL_SIZE { 100 };

// Chunk size in voxels
static constexpr int CHUNK_SIZE { 16 };

// Chunk size in unreal units
static constexpr int CHUNK_SIZE_UNREAL { VOXEL_SIZE * CHUNK_SIZE };

// Number of voxel in each chunk
static constexpr int NUM_VOXELS_IN_CHUNK { CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE };

// Chunks at this distance and beyond will not receive updates from the server
static constexpr int STALE_DISTANCE { 8 };

// Upper limit for Load/Render distance parameters
static constexpr int MAX_LOAD_DISTANCE { 100U };

static constexpr int64 DEFAULT_MAP_ID { 0 };