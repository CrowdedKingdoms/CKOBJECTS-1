// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelPlacementComponent.h"

#include "GameFramework/Character.h"
#include "Voxels/Components/VoxelManager.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"

UVoxelPlacementComponent::UVoxelPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}


void UVoxelPlacementComponent::BeginPlay()
{
	Super::BeginPlay();

	MyOwner = Cast<AActor>(GetOwner());
	
	UE_LOG(LogTemp, Log, TEXT("VoxelPlacementComponent Activated: %s"), *MyOwner->GetName());
}

void UVoxelPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}




FVector UVoxelPlacementComponent::GetLineTraceStartPoint() const
{
	FVector StartLocation = FVector::ZeroVector;
	
	if (IsValid(MyOwner.Get()))
	{
		StartLocation = MyOwner->GetActorLocation();
        
		StartLocation.Z += 50.0f;
	}
    
	return StartLocation;
}


float UVoxelPlacementComponent::CheckPlayerProximity(const FVector& HitLocation) const
{
	if (IsValid(MyOwner.Get()))
	{
		FVector PlayerLocation = MyOwner->GetActorLocation();

		float Distance = FVector::Dist(PlayerLocation, HitLocation);

		return (Distance <= 150.0f);
	}

	return false;
}


void UVoxelPlacementComponent::CreateVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ)
{
	if ( !VoxelServiceSubsystem || !VoxelWorldSubsystem || !VoxelManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelService or VoxelWorldSubsystem or VoxelManager is not valid"));
		return;
	}

	AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(ChunkX, ChunkY, ChunkZ);
	
	if (IsValid(VoxelChunk))
	{
		uint8 TempVoxelType = VoxelChunk->GetVoxel(VoxelX, VoxelY, VoxelZ);
		VoxelManager->SetVoxelType(TempVoxelType);
		VoxelChunk->UpdateVoxel(VoxelX, VoxelY, VoxelZ, static_cast<uint8>(VoxelManager->CurrentVoxelType));
	}
	else
	{
		TArray<FChunkVoxelState> VoxelStates;
		VoxelWorldSubsystem->CreateVoxelChunk(ChunkX, ChunkY, ChunkZ, VoxelWorldSubsystem->GetFullArray(0), VoxelStates);
		CreateVoxel(ChunkX,ChunkY,ChunkZ,VoxelX,VoxelY,VoxelZ);
	}
}




FVoxelState UVoxelPlacementComponent::CreateVLO(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ)
{
	if ( !VoxelServiceSubsystem || !VoxelWorldSubsystem || !VoxelManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelService or VoxelWorldSubsystem or VoxelManager is not valid"));
		return FVoxelState();
	}

	AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(ChunkX, ChunkY, ChunkZ);
	
	if (IsValid(VoxelChunk))
	{
		uint8 TempVoxelType = VoxelChunk->GetVoxel(VoxelX, VoxelY, VoxelZ);
		VoxelManager->SetVoxelType(TempVoxelType);
		FVoxelState& TempVoxelState = VoxelChunk->GetVoxelState(VoxelX, VoxelY, VoxelZ);
		TempVoxelState.Version = 5;
		TempVoxelState.bIsVLO = true;
		
		VoxelChunk->UpdateVoxel(VoxelX, VoxelY, VoxelZ, static_cast<uint8>(VoxelManager->CurrentVoxelType));
		
		return TempVoxelState;
	}
	else
	{
		TArray<FChunkVoxelState> VoxelStates;
		VoxelWorldSubsystem->CreateVoxelChunk(ChunkX, ChunkY, ChunkZ, VoxelWorldSubsystem->GetFullArray(0), VoxelStates);
		CreateVLO(ChunkX,ChunkY,ChunkZ,VoxelX,VoxelY,VoxelZ);
	}
	return FVoxelState();
}