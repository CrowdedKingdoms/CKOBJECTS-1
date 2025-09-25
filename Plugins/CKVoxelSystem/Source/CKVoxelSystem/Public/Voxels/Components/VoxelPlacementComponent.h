// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "VoxelPlacementComponent.generated.h"

class UVoxelManager;
class UVoxelServiceSubsystem;
class UVoxelWorldSubsystem;


UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class UVoxelPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVoxelPlacementComponent();
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelPlacement")
	UVoxelManager* VoxelManager = nullptr;
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void CreateVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ);
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	FVoxelState CreateVLO(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ);
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	FVoxelState CreateGameObject(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ);
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void RemoveVoxelOrVLO(FVector Location);
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void RemoveObject(UInstancedStaticMeshComponent* ISMComponent, int32 InstanceIndex);

	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void RemoveVoxel();

	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void PlaceVoxel();
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void PlaceGameObject();
	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void PlaceVLO();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelPlacement")
	UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelPlacement")
	UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;

private:

	TWeakObjectPtr<AActor> MyOwner;
	
	FVector GetLineTraceStartPoint() const;
	float CheckPlayerProximity(const FVector& HitLocation) const;
};
