// Fill out your copyright notice in the Description page of Project Settings.



#include "GameObjects/Framework/Base/ActivatorBase.h"
#include "GameObjects/Framework/Management/GameObjectsManager.h"
#include "Kismet/GameplayStatics.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"


// Sets default values
AActivatorBase::AActivatorBase(): TargetObject(nullptr), MapID(0),ChunkX(0), ChunkY(0), ChunkZ(0),
                                  GameObjectsManager(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AActivatorBase::BeginPlay()
{
	Super::BeginPlay();

	OriginalLocation = GetActorLocation();
	
	if (!IsValid(TargetObject))
	{
		UE_LOG(LogTemp, Error, TEXT("ActivatorBaseObject: TargetObject is not valid"));
	}

	UVoxelWorldSubsystem* VoxelWorldController = UVoxelWorldSubsystem::StaticClass()->GetDefaultObject<UVoxelWorldSubsystem>();

	if (IsValid(VoxelWorldController))
	{
		VoxelWorldController->CalculateChunkCoordinatesAtWorldLocation(this->GetActorLocation(), ChunkX, ChunkY, ChunkZ);
		UE_LOG(LogTemp, Log, TEXT("Activator's ChunkCoords set to %lld, %lld, %lld"), ChunkX, ChunkY, ChunkZ);
	}
}

// Called every frame
void AActivatorBase::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AActivatorBase::ActivateEvent_Implementation(const FGameObjectState& NewState)
{
	UE_LOG(LogTemp, Log, TEXT("ActivatorBaseObject: ActivateEvent Called"));
	UE_LOG(LogTemp, Log, TEXT("ActivatorBaseObject: Game Object State Changed"));
	TargetObject->ActivateGameObject(NewState);
}

void AActivatorBase::SetGameObjectManagerReference(AGameObjectsManager* InGameObjectManager)
{
	GameObjectsManager = InGameObjectManager;
}

void AActivatorBase::SetActivatorUUID(const FString& inUUID)
{
	ActivatorUUID = inUUID;
}

void AActivatorBase::RequestActivation_Implementation()
{
	FGameObjectState NewState;
	NewState.bIsActive = true;
	const uint16 EType = static_cast<uint16>(EventType);
	GameObjectsManager->DispatchActivationRequest(MapID, ChunkX, ChunkY, ChunkZ, ActivatorUUID, EType, NewState);
}

void AActivatorBase::RequestActivationWithState_Implementation(const FGameObjectState& NewState)
{
	const uint16 EType = static_cast<uint16>(EventType);
	GameObjectsManager->DispatchActivationRequest(MapID, ChunkX, ChunkY, ChunkZ, ActivatorUUID, EType, NewState);
}

AGameObjectBase* AActivatorBase::GetAssociatedObject_Implementation()
{
	return TargetObject;
}

FVector AActivatorBase::GetActorOriginalLocation_Implementation()
{
	return OriginalLocation;
}
