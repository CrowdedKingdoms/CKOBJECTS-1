// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shared/Types/Enums/Voxels/EVoxelType.h"
#include "VoxelManager.generated.h"


class AGhostPlacement;
class APlayerController;
class AChunkVoxelManager;	
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
	float Delta = 0.0005f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Material")
	UMaterialInterface* ExternalGhostMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Values")
	FName SelectedObjectId = FName("");
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Values")
	float InteractRadios = 800.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager | Voxel")
	EVoxelType CurrentVoxelType;

	UFUNCTION(BlueprintCallable, Category="VoxelManager")
	void InitializeVoxelSystem();
	

	UFUNCTION(BlueprintCallable, Category = "VoxelManager | Refs Get")
	UVoxelPlacementComponent* GetVoxelPlacementComponentRef() { return  PlacementComp;}	
	
	UFUNCTION(BlueprintCallable, Category = "VoxelManager | Refs Get")
	UVoxelRotationComponent* GetVoxelRotationComponentRef() { return  RotationComp;}	

	UFUNCTION(BlueprintCallable, Category = "VoxelManager | Refs Get")
	AGhostPlacement* GetGhostPlacementRef() { return  GhostPreview;}	
	
	UFUNCTION(BlueprintCallable, Category = "VoxelManager | Refs Get")
	AChunkVoxelManager* GetChunkVoxelManagerRef() { return  ChunkVoxelManager;}

	UFUNCTION(BlueprintCallable, Category = "VoxelManager | Refs Get")
	APlayerController* GetPlayerControllerRef() { return  PlayerController;}
	
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelManager")
	APlayerController* PlayerController;

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

	UPROPERTY(Transient)
	AChunkVoxelManager* ChunkVoxelManager;
	
};
