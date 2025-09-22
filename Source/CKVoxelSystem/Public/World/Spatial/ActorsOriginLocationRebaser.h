// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActorsOriginLocationRebaser.generated.h"

class UVoxelWorldSubsystem;

UCLASS(blueprintable, BlueprintType)
class  AActorsOriginLocationRebaser : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AActorsOriginLocationRebaser();

	UFUNCTION(BlueprintCallable, Category = "Actors Origin Location Rebaser")
	void SetVoxelWorldController(UVoxelWorldSubsystem* WorldController);

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Actors Origin Location Rebaser")
	void RebaseActorLocations();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actors Origin Location Rebaser")
	FVector OriginalLocation;

	UPROPERTY()
	UVoxelWorldSubsystem* VoxelWorldController;

	UPROPERTY()
	TArray<AActor*> ActorsToRebase;
};
