// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoxelManager.generated.h"

class ACharacter;
class UVoxelServiceSubsystem;
class UVoxelWorldSubsystem;
class UVoxelPlacementComponent;
class UVoxelRotationComponent;


UCLASS(ClassGroup=(VoxelCore), Blueprintable, meta=(BlueprintSpawnableComponent))
class UVoxelManager : public UActorComponent
{
	GENERATED_BODY()

public:

	UVoxelManager();

	/** Optional manual subsystem refs (if not set, auto-fills in BeginPlay) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelCore")
	UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelCore")
	UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelCore|Setup")
	bool bEnablePlacement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelCore|Setup")
	bool bEnableRotation = true;

protected:
	virtual void BeginPlay() override;

private:
	/** Cached owning character */
	TWeakObjectPtr<AActor> Owner;

	/** Sub-feature components created at runtime */
	UPROPERTY(Transient)
	UVoxelPlacementComponent* PlacementComp;

	UPROPERTY(Transient)
	UVoxelRotationComponent* RotationComp;
	
	UFUNCTION(BlueprintCallable, Category="VoxelManager")
	bool InitializeVoxelSystem(
		UVoxelServiceSubsystem* InService, UVoxelWorldSubsystem* InWorld, bool bEnablePlacementFeature, bool bEnableRotationFeature);

	
};
