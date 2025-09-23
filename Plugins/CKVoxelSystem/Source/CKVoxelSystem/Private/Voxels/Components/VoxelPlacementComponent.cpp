// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelPlacementComponent.h"

#include "GameFramework/Character.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"

UVoxelPlacementComponent::UVoxelPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}


void UVoxelPlacementComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AActor>(GetOwner());
	
	UE_LOG(LogTemp, Log, TEXT("VoxelPlacementComponent Activated: %s"), *Owner->GetName());
}

void UVoxelPlacementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}




FVector UVoxelPlacementComponent::GetLineTraceStartPoint() const
{
	FVector StartLocation = FVector::ZeroVector;
	
	if (IsValid(Owner.Get()))
	{
		StartLocation = Owner->GetActorLocation();
        
		StartLocation.Z += 50.0f;
	}
    
	return StartLocation;
}


float UVoxelPlacementComponent::CheckPlayerProximity(const FVector& HitLocation) const
{
	if (IsValid(Owner.Get()))
	{
		FVector PlayerLocation = Owner->GetActorLocation();

		float Distance = FVector::Dist(PlayerLocation, HitLocation);

		return (Distance <= 150.0f);
	}

	return false;
}


void UVoxelPlacementComponent::CreateVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ)
{
	if ( VoxelServiceSubsystem == nullptr || VoxelWorldSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelService or VoxelWorldSubsystem is not valid"));
		return;
	}
	//TODO: 
	// Getting the chunk manager
	// AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(ChunkX, ChunkY, ChunkZ);
	// if (VoxelChunk)
	// {
	//
	// 	const uint8  VoxelChunk->GetVoxel(VoxelX, VoxelY, VoxelZ);
	// 	
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("VoxelChunk is not valid"));
	//
	// 	TArray<FChunkVoxelState> VoxelState;
	// 	VoxelWorldSubsystem->CreateVoxelChunk(ChunkX, ChunkY, ChunkZ, VoxelWorldSubsystem->GetFullArray(0), VoxelState);
	// 	
	// }
	//
	//
	//
	// VoxelChunk->UpdateVoxel(VoxelX, VoxelY, VoxelZ, int8(EVoxelType::RED));
}
