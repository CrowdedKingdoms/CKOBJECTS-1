// Fill out your copyright notice in the Description page of Project Settings.

#include "World/Spatial//ActorsOriginLocationRebaser.h"
#include "Kismet/GameplayStatics.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"
#include "Shared/Types/Interfaces//World/OriginRebasable.h"

// Sets default values
AActorsOriginLocationRebaser::AActorsOriginLocationRebaser(): VoxelWorldController(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AActorsOriginLocationRebaser::SetVoxelWorldController(UVoxelWorldSubsystem* WorldController)
{
	VoxelWorldController = WorldController;
	VoxelWorldController->OnOriginRebased.AddDynamic(this, &AActorsOriginLocationRebaser::RebaseActorLocations);
}

// Called when the game starts or when spawned
void AActorsOriginLocationRebaser::BeginPlay()
{
	Super::BeginPlay();
	
}

void AActorsOriginLocationRebaser::RebaseActorLocations()
{
	
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UOriginRebasable::StaticClass(), ActorsToRebase);
	
	const int64 X = VoxelWorldController->GetOriginOffset().OffsetX;
	const int64 Y = VoxelWorldController->GetOriginOffset().OffsetY;
	const int64 Z = VoxelWorldController->GetOriginOffset().OffsetZ;
	
	for (AActor* Actor : ActorsToRebase)
	{
		FVector Lcl_OriginalLocation = IOriginRebasable::Execute_GetActorOriginalLocation(Actor);
		FVector NewLocation = Lcl_OriginalLocation - FVector(X, Y, Z);
		Actor->SetActorLocation(NewLocation);
	}
}

// Called every frame
void AActorsOriginLocationRebaser::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

