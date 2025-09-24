// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelRotationComponent.h"

#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


UVoxelRotationComponent::UVoxelRotationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVoxelRotationComponent::BeginPlay()
{
    Super::BeginPlay();
    MyOwner = Cast<AActor>(GetOwner());
	
    UE_LOG(LogTemp, Log, TEXT("VoxelRotationComponent Activated: %s"), *MyOwner->GetName());
}
