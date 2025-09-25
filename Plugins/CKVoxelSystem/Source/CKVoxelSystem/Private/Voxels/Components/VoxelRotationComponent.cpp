// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelRotationComponent.h"

#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "FunctionLibraries/Generic/FL_Generic.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"


UVoxelRotationComponent::UVoxelRotationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVoxelRotationComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!GetWorld()) return;
    
    MyOwner = Cast<AActor>(GetOwner());
    VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
    VoxelWorldSubsystem = GetWorld()->GetSubsystem<UVoxelWorldSubsystem>();
	
    UE_LOG(LogTemp, Log, TEXT("VoxelRotationComponent Activated: Owner=%s, VoxelServiceSubsystem=%s, VoxelWorldSubsystem=%s"), *MyOwner->GetName(), *VoxelServiceSubsystem->GetName(), *VoxelWorldSubsystem->GetName());

}
