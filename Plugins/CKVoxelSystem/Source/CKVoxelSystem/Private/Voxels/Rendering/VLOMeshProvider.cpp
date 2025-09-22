// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Rendering/VLOMeshProvider.h"

DEFINE_LOG_CATEGORY(LogVLOMeshProvider);


// Sets default values
AVLOMeshProvider::AVLOMeshProvider()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AVLOMeshProvider::BeginPlay()
{
	Super::BeginPlay();

	// Filling empty VLO types with base mesh
	for (int32 i = 12; i < 256; i++)
	{
		SetVLOMesh(static_cast<uint8>(i), DefaultVLOMesh);
	}

	
	UE_LOG(LogVLOMeshProvider, Log, TEXT("VLO Mesh Provider initialized with %d mesh mappings"), VLOMeshMappings.Num());
}


// Called every frame
void AVLOMeshProvider::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UStaticMesh* AVLOMeshProvider::GetVLOMesh(uint8 VoxelType) const
{
	if (UStaticMesh* const* FoundMesh = VLOMeshMappings.Find(VoxelType))
	{
		return *FoundMesh;
	}
	return nullptr;
}

bool AVLOMeshProvider::HasVLOMesh(uint8 VoxelType) const
{
	return VLOMeshMappings.Contains(VoxelType);
}


void AVLOMeshProvider::SetVLOMesh(uint8 VoxelType, UStaticMesh* Mesh)
{
	if (Mesh)
	{
		VLOMeshMappings.Add(VoxelType, Mesh);
		UE_LOG(LogVLOMeshProvider, Log, TEXT("Set VLO mesh for voxel type %d: %s"), VoxelType, *Mesh->GetName());
	}
	else
	{
		VLOMeshMappings.Remove(VoxelType);
		UE_LOG(LogVLOMeshProvider, Log, TEXT("Removed VLO mesh for voxel type %d"), VoxelType);
	}
}


void AVLOMeshProvider::RemoveVLOMesh(uint8 VoxelType)
{
	VLOMeshMappings.Remove(VoxelType);
}


