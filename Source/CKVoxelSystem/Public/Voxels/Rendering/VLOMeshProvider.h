// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VLOMeshProvider.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVLOMeshProvider, Log, All);

UCLASS(Blueprintable, BlueprintType)
class   AVLOMeshProvider : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AVLOMeshProvider();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	// Simple mesh lookup functions
	UFUNCTION(BlueprintCallable, Category = "VLO")
	UStaticMesh* GetVLOMesh(uint8 VoxelType) const;

	UFUNCTION(BlueprintCallable, Category = "VLO")
	bool HasVLOMesh(uint8 VoxelType) const;

	// Functions for setting up mappings
	UFUNCTION(BlueprintCallable, Category = "VLO")
	void SetVLOMesh(uint8 VoxelType, UStaticMesh* Mesh);

	UFUNCTION(BlueprintCallable, Category = "VLO")
	void RemoveVLOMesh(uint8 VoxelType);

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Simple map: voxel type -> VLO mesh (editable in Blueprint/Editor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VLO Meshes")
	TMap<uint8, UStaticMesh*> VLOMeshMappings;

	UPROPERTY(EditAnywhere)
	UStaticMesh* DefaultVLOMesh;
	
};
