// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelManager.h"

#include "GameFramework/Character.h"
#include "Voxels/Components/VoxelPlacementComponent.h"
#include "Voxels/Components/VoxelRotationComponent.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Core/ChunkVoxelManager.h"
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

    VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
    VoxelWorldSubsystem = GetWorld()->GetSubsystem<UVoxelWorldSubsystem>();
    

    // Placement
    if (bEnablePlacement && !PlacementComp)
    {
        PlacementComp = Cast<UVoxelPlacementComponent>(
            MyOwner->AddComponentByClass(UVoxelPlacementComponent::StaticClass(), false, FTransform::Identity, false)
        );
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

    if (!ChunkVoxelManager)
    {
        ChunkVoxelManager = GetWorld()->SpawnActor<AChunkVoxelManager>(AChunkVoxelManager::StaticClass(), MyOwner->GetActorTransform());
        ChunkVoxelManager->MyOwner = MyOwner;
    }
    
    
    UE_LOG(LogTemp, Log,
        TEXT("VoxelManager: Initialized (Service=%s, World=%s, Placement=%s, Rotation=%s)"),
        *GetNameSafe(VoxelServiceSubsystem),
        *GetNameSafe(VoxelWorldSubsystem),
        bEnablePlacement ? TEXT("Enabled") : TEXT("Disabled"),
        bEnableRotation ? TEXT("Enabled") : TEXT("Disabled"));

}





