// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "VoxelPlacementComponent.generated.h"

class UVoxelServiceSubsystem;
class UVoxelWorldSubsystem;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CKVOXELSYSTEM_API UVoxelPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVoxelPlacementComponent();

	// In VoxelRotationComponent.h / VoxelPlacementComponent.h

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelPlacement")
	UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelPlacement")
	UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;

	
	UFUNCTION(BlueprintCallable, Category="VoxelPlacement")
	void CreateVoxel(int64 ChunkX, int64 ChunkY, int64 ChunkZ, int VoxelX, int VoxelY, int VoxelZ);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:

	TWeakObjectPtr<AActor> Owner;
	
	FVector GetLineTraceStartPoint() const;
	float CheckPlayerProximity(const FVector& HitLocation) const;
};
