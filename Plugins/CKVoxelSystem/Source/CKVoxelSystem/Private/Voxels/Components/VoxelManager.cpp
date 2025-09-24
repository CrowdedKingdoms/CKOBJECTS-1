// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelManager.h"

#include "GameFramework/Character.h"
#include "Voxels/Components/VoxelPlacementComponent.h"
#include "Voxels/Components/VoxelRotationComponent.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Core/GhostPlacement.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"


UVoxelManager::UVoxelManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UVoxelManager::BeginPlay()
{
	Super::BeginPlay();

	// Cache owning character
	MyOwner = Cast<AActor>(GetOwner());
	if (!MyOwner.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelFeatureCoreComponent: Owner is not a Character!"));
		return;
	}

    InitializeVoxelSystem();
}

void UVoxelManager::InitializeVoxelSystem()
{
    if (!GetWorld() || !MyOwner.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("VoxelManager: Initialize failed (no world or owner)."));
        return;
    }

    // Resolve subsystems (manual or auto)
    VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
    VoxelWorldSubsystem = GetWorld()->GetSubsystem<UVoxelWorldSubsystem>();

    // Update existing sub-feature refs
    if (PlacementComp)
    {
        PlacementComp->VoxelServiceSubsystem = VoxelServiceSubsystem;
        PlacementComp->VoxelWorldSubsystem = VoxelWorldSubsystem;
    }
    if (RotationComp)
    {
        RotationComp->VoxelServiceSubsystem = VoxelServiceSubsystem;
        RotationComp->VoxelWorldSubsystem = VoxelWorldSubsystem;
    }

    // Placement
    if (bEnablePlacement && !PlacementComp)
    {
        PlacementComp = Cast<UVoxelPlacementComponent>(
            MyOwner->AddComponentByClass(UVoxelPlacementComponent::StaticClass(), false, FTransform::Identity, false)
        );
        if (PlacementComp)
        {
            PlacementComp->VoxelServiceSubsystem = VoxelServiceSubsystem;
            PlacementComp->VoxelWorldSubsystem = VoxelWorldSubsystem;
        }
    }
    else if (!bEnablePlacement && PlacementComp)
    {
        PlacementComp->DestroyComponent();
        PlacementComp = nullptr;
    }

    // Rotation
    if (bEnableRotation && !RotationComp)
    {
        RotationComp = Cast<UVoxelRotationComponent>(
            MyOwner->AddComponentByClass(UVoxelRotationComponent::StaticClass(), false, FTransform::Identity, false)
        );
        if (RotationComp)
        {
            RotationComp->VoxelServiceSubsystem = VoxelServiceSubsystem;
            RotationComp->VoxelWorldSubsystem = VoxelWorldSubsystem;
        }
    }
    else if (!bEnableRotation && RotationComp)
    {
        RotationComp->DestroyComponent();
        RotationComp = nullptr;
    }

    if (bEnableGhostPreview && !GhostPreview && ExternalGhostMaterial)
    {
        GhostPreview = GetWorld()->SpawnActor<AGhostPlacement>(AGhostPlacement::StaticClass(), MyOwner->GetActorTransform());
        GhostPreview->MyOwner = MyOwner;
        GhostPreview->GhostMaterialReference = ExternalGhostMaterial;
    }

    UE_LOG(LogTemp, Log,
        TEXT("VoxelManager: Initialized (Service=%s, World=%s, Placement=%s, Rotation=%s)"),
        *GetNameSafe(VoxelServiceSubsystem),
        *GetNameSafe(VoxelWorldSubsystem),
        bEnablePlacement ? TEXT("Enabled") : TEXT("Disabled"),
        bEnableRotation ? TEXT("Enabled") : TEXT("Disabled"));

}


void UVoxelManager::ChunkVoxelManager()
{
    VoxelServiceSubsystem->OnVoxelUpdateResponse.AddDynamic(this, &UVoxelManager::RemoveVoxel);
    VoxelServiceSubsystem->OnNewVoxelUpdateNotification.AddDynamic(this, &UVoxelManager::OnNewVoxelUpdate);
}

void UVoxelManager::RemoveVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int32 Vx, int32 Vy, int32 Vz)
{
    AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(ChunkX, ChunkY, ChunkZ);
    if (VoxelChunk)
    {
        VoxelChunk->UpdateVoxel(Vx, Vy, Vz, static_cast<uint8>(TypeOfVoxel));
    }
}

void UVoxelManager::OnNewVoxelUpdate(int64 Cx, int64 Cy, int64 Cz, int32 Vx, int32 Vy, int32 Vz, uint8 VoxelType, FVoxelState VoxelState, bool bHasState)
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


void UVoxelManager::SetVoxelType(uint8 VoxelType)
{
    TypeOfVoxel = static_cast<EVoxelType>(VoxelType);
}



