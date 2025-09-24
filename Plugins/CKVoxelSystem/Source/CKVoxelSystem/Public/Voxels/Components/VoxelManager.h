// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shared/Types/Enums/Voxels/EVoxelType.h"
#include "VoxelManager.generated.h"

class AGhostPlacement;
class UVoxelWorldSubsystem;
class UVoxelServiceSubsystem;
class UVoxelPlacementComponent;
class UVoxelRotationComponent;


UCLASS(ClassGroup=(VoxelCore), Blueprintable, meta=(BlueprintSpawnableComponent))
class UVoxelManager : public UActorComponent
{
	GENERATED_BODY()

public:

	UVoxelManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager")
	UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager")
	UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Setup")
	bool bEnablePlacement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Setup")
	bool bEnableRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Setup")
	bool bEnableGhostPreview = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Setup")
	UMaterialInterface* ExternalGhostMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Voxel")
	EVoxelType CurrentVoxelType;

	UFUNCTION(BlueprintCallable, Category="VoxelManager")
	void InitializeVoxelSystem();

	UFUNCTION(BlueprintCallable, Category="VoxelManager")	
	void SetVoxelType(uint8 VoxelType);
	
	UFUNCTION(BlueprintCallable, Category="VoxelManager")
	void ChunkVoxelManager();

	UFUNCTION(BlueprintCallable, Category = "VoxelManager")
	void RemoveVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int32 Vx, int32 Vy, int32 Vz);

	UFUNCTION(BlueprintCallable, Category = "VoxelManager")
	void OnNewVoxelUpdate(int64 Cx, int64 Cy, int64 Cz, int32 Vx, int32 Vy, int32 Vz, uint8 VoxelType, FVoxelState VoxelState, bool bHasState);
	

	
	
protected:
	virtual void BeginPlay() override;

private:
	/** Cached owning character */
	TWeakObjectPtr<AActor> MyOwner;

	/** Sub-feature components created at runtime */
	UPROPERTY(Transient)
	UVoxelPlacementComponent* PlacementComp;

	UPROPERTY(Transient)
	UVoxelRotationComponent* RotationComp;
	
	UPROPERTY(Transient)
	AGhostPlacement* GhostPreview;

	EVoxelType TypeOfVoxel;
};
