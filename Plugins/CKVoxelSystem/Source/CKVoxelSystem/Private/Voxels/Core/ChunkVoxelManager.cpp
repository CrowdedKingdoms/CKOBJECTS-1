
#include "Voxels/Core/ChunkVoxelManager.h"
#include "FunctionLibraries/Generic/FL_Generic.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"

AChunkVoxelManager::AChunkVoxelManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AChunkVoxelManager::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld()) return;


	VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	VoxelWorldSubsystem = GetWorld()->GetSubsystem<UVoxelWorldSubsystem>();
	UE_LOG(LogTemp, Log, TEXT("AChunkVoxelManager Activated: VoxelServiceSubsystem=%s, VoxelWorldSubsystem=%s"),*VoxelServiceSubsystem->GetName(), *VoxelWorldSubsystem->GetName());
	ChunkVoxelManager();

	
}


void AChunkVoxelManager::ChunkVoxelManager()
{
	VoxelServiceSubsystem->OnVoxelUpdateResponse.AddDynamic(this, &AChunkVoxelManager::RemoveVoxel);
	VoxelServiceSubsystem->OnNewVoxelUpdateNotification.AddDynamic(this, &AChunkVoxelManager::OnNewVoxelUpdate);
}

void AChunkVoxelManager::RemoveVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int32 Vx, int32 Vy, int32 Vz)
{
	AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(ChunkX, ChunkY, ChunkZ);
	if (VoxelChunk)
	{
		VoxelChunk->UpdateVoxel(Vx, Vy, Vz, static_cast<uint8>(TypeOfVoxel));
	}
}

void AChunkVoxelManager::OnNewVoxelUpdate(int64 Cx, int64 Cy, int64 Cz, int32 Vx, int32 Vy, int32 Vz, uint8 VoxelType, FVoxelState VoxelState, bool bHasState)
{
	AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(Cx, Cy, Cz);

	if (bHasState)
	{
		VoxelChunk->UpdateVoxelWithState(Vx, Vy, Vz, VoxelType, VoxelState);
	}
	else
	{
		VoxelChunk->UpdateVoxel(Vx, Vy, Vz, VoxelType);
	}
}


void AChunkVoxelManager::SetVoxelType(uint8 VoxelType)
{
	TypeOfVoxel = static_cast<EVoxelType>(VoxelType);
}
