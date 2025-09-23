// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelManager.h"

#include "GameFramework/Character.h"
#include "Voxels/Components/VoxelPlacementComponent.h"
#include "Voxels/Components/VoxelRotationComponent.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"


UVoxelManager::UVoxelManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UVoxelManager::BeginPlay()
{
	Super::BeginPlay();

	// Cache owning character
	Owner = Cast<ACharacter>(GetOwner());
	if (!Owner.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelFeatureCoreComponent: Owner is not a Character!"));
		return;
	}
    
}

bool UVoxelManager::InitializeVoxelSystem(
    UVoxelServiceSubsystem* InService,
    UVoxelWorldSubsystem* InWorld,
    bool bEnablePlacementFeature,
    bool bEnableRotationFeature
)
{
    if (!GetWorld() || !Owner.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("VoxelManager: Initialize failed (no world or owner)."));
        return false;
    }

    // Resolve subsystems (manual or auto)
    VoxelServiceSubsystem = InService ? InService : GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
    VoxelWorldSubsystem = InWorld ? InWorld : GetWorld()->GetSubsystem<UVoxelWorldSubsystem>();

    // Store feature toggles
    bEnablePlacement = bEnablePlacementFeature;
    bEnableRotation  = bEnableRotationFeature;

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
            Owner->AddComponentByClass(UVoxelPlacementComponent::StaticClass(), false, FTransform::Identity, false)
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
            Owner->AddComponentByClass(UVoxelRotationComponent::StaticClass(), false, FTransform::Identity, false)
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

    UE_LOG(LogTemp, Log,
        TEXT("VoxelManager: Initialized (Service=%s, World=%s, Placement=%s, Rotation=%s)"),
        *GetNameSafe(VoxelServiceSubsystem),
        *GetNameSafe(VoxelWorldSubsystem),
        bEnablePlacement ? TEXT("Enabled") : TEXT("Disabled"),
        bEnableRotation ? TEXT("Enabled") : TEXT("Disabled"));

    return true;
}


