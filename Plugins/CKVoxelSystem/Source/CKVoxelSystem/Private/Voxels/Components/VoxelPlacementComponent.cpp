// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Components/VoxelPlacementComponent.h"

#include "FunctionLibraries/Generic/FL_Generic.h"
#include "GameFramework/Character.h"
#include "Network/Services/GameData/VoxelServiceSubsystem.h"
#include "Voxels/Components/VoxelManager.h"
#include "Voxels/Core/ChunkVoxelManager.h"
#include "Voxels/Core/GhostPlacement.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"

UVoxelPlacementComponent::UVoxelPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}


void UVoxelPlacementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld()) return;

	MyOwner = Cast<AActor>(GetOwner());
	VoxelServiceSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	VoxelWorldSubsystem = GetWorld()->GetSubsystem<UVoxelWorldSubsystem>();
	
	UE_LOG(LogTemp, Log, TEXT("VoxelPlacementComponent Activated: Owner=%s, VoxelServiceSubsystem=%s, VoxelWorldSubsystem=%s"), *MyOwner->GetName(), *VoxelServiceSubsystem->GetName(), *VoxelWorldSubsystem->GetName());
	
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
		VoxelManager->GetChunkVoxelManagerRef()->SetVoxelType(TempVoxelType);
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
		VoxelManager->GetChunkVoxelManagerRef()->SetVoxelType(TempVoxelType);
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

FVoxelState UVoxelPlacementComponent::CreateGameObject(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ)
{
    if (!VoxelServiceSubsystem || !VoxelWorldSubsystem || !VoxelManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("VoxelService or VoxelWorldSubsystem or VoxelManager is not valid"));
        return FVoxelState();
    }

    AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(ChunkX, ChunkY, ChunkZ);

    if (IsValid(VoxelChunk))
    {
        AGhostPlacement* TempGhostPreviewRef = VoxelManager->GetGhostPlacementRef();
        if (!IsValid(TempGhostPreviewRef))
        {
            UE_LOG(LogTemp, Warning, TEXT("CreateGameObject: GhostPlacement ref is not valid"));
            return FVoxelState();
        }

        FPlacedObjectState NewPlaced;

        // If FPlacedObjectState::ObjectID is FString:
        NewPlaced.ObjectID = VoxelManager->SelectedObjectId.ToString();

        // If ObjectID is FName, prefer:
        // NewPlaced.ObjectID = VoxelManager->SelectedObjectId;

        NewPlaced.Location = TempGhostPreviewRef->GetActorLocation() - VoxelChunk->GetVoxelCenter(FVector(VoxelX, VoxelY, VoxelZ));
        NewPlaced.Rotation = TempGhostPreviewRef->GetActorRotation();
        NewPlaced.Scale = TempGhostPreviewRef->GetActorScale();

        FVoxelState VState = VoxelChunk->GetVoxelState(VoxelX, VoxelY, VoxelZ);
        VState.Version = 5;
        VState.GameObjects.Add(NewPlaced);

        // Make sure the Update function signature matches your project:
        VoxelChunk->UpdateVoxelWithState(VoxelX, VoxelY, VoxelZ, static_cast<uint8>(VoxelManager->CurrentVoxelType), VState);

        return VState;
    }
    else
    {
        TArray<FChunkVoxelState> VoxelStates;
        VoxelWorldSubsystem->CreateVoxelChunk(ChunkX, ChunkY, ChunkZ, VoxelWorldSubsystem->GetFullArray(0), VoxelStates);

        // Return the result of the recursive call so the caller receives the created state.
        return CreateGameObject(ChunkX, ChunkY, ChunkZ, VoxelX, VoxelY, VoxelZ);
    }

    // unreachable, but keep for clarity
    return FVoxelState();
}



void UVoxelPlacementComponent::RemoveVoxelOrVLO(FVector Location)
{

	if (!VoxelServiceSubsystem || !VoxelWorldSubsystem || !VoxelManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("VoxelService or VoxelWorldSubsystem or VoxelManager is not valid"));
	}
	
	int32 Vx = 0, Vy = 0, Vz = 0;
	int64 Cx = 0, Cy = 0, Cz = 0;
	
	UFL_Generic::CalculateVoxelCoordinatesAtWorldLocation(Location,Vx, Vy, Vz);

	VoxelWorldSubsystem->CalculateChunkCoordinatesAtWorldLocation(Location, Cx, Cy, Cz);

	AVoxelChunk* VoxelChunk = VoxelWorldSubsystem->GetChunk(Cx, Cy, Cz);
	if (!VoxelChunk) return;
	
	uint8 TempVoxelType = VoxelChunk->GetVoxel(Vx, Vy, Vz);
	VoxelManager->GetChunkVoxelManagerRef()->SetVoxelType(TempVoxelType);
	
	FVoxelState VState;
	VState.Version = 5;
	VState.bIsVLO = false;
	VState.FaceOneDirection = 0;
	VState.Rotation = 0;
	
	VoxelChunk->UpdateVoxelWithState(Vx, Vy, Vz, 0, VState);

	VoxelServiceSubsystem->SendVoxelStateUpdateRequest(Cx, Cy, Cz, Vx, Vy, Vz, 0, FVoxelState(), false);
}


void UVoxelPlacementComponent::RemoveObject(UInstancedStaticMeshComponent* ISMComponent, int32 InstanceIndex)
{

	if (!IsValid(ISMComponent) || !VoxelManager || !VoxelWorldSubsystem || !VoxelServiceSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetInstanceWorldLocation: ISMComponent is null/invalid"));
		return;
	}

	FTransform InstanceTransform;
	ISMComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, /*bWorldSpace=*/true);
	FVector InstanceLocation = InstanceTransform.GetLocation();
	
	AVoxelChunk* VoxelChunk = Cast<AVoxelChunk>(ISMComponent->GetOwner());
	
	if (!VoxelChunk)
	{
		UE_LOG(LogTemp, Warning, TEXT("FAILED TO GET VOXEL CHUNK"));
		return;
	}

	int32 Vx = 0, Vy = 0, Vz = 0;
	UFL_Generic::CalculateVoxelCoordinatesAtWorldLocation(InstanceLocation, Vx, Vy, Vz);

	FVoxelState VState = VoxelChunk->GetVoxelState(Vx, Vy, Vz);
	const uint8  VType = VoxelChunk->GetVoxel(Vx,Vy, Vz);

	FVector VoxelCenter = VoxelChunk->GetVoxelCenter(FVector(Vx, Vy, Vz));
	
	for (FPlacedObjectState object : VState.GameObjects)	
	{
		float Tolerance = VoxelManager->Delta;

		FVector Target = object.Location + VoxelCenter;
		if (InstanceLocation.Equals(Target, Tolerance))
		{
			VState.GameObjects.Remove(object);
			break;
		}
	}
	int64 Cx = 0, Cy = 0, Cz = 0;
	VoxelWorldSubsystem->CalculateChunkCoordinatesAtWorldLocation(InstanceLocation, Cx, Cy, Cz);
	
	VoxelServiceSubsystem->SendVoxelStateUpdateRequest(Cx, Cy, Cz, Vx, Vy, Vz, VType, VState, true);

	ISMComponent->RemoveInstance(InstanceIndex);
	
}

void UVoxelPlacementComponent::RemoveVoxel()
{
	if (!VoxelManager || !VoxelWorldSubsystem || !VoxelServiceSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetInstanceWorldLocation: ISMComponent is null/invalid"));
		return;
	}
	
}

void UVoxelPlacementComponent::PlaceVoxel()
{
	
}


void UVoxelPlacementComponent::PlaceGameObject()
{
	
}

void UVoxelPlacementComponent::PlaceVLO()
{
	
}