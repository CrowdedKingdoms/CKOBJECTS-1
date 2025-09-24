// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostPlacement.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(Blueprintable, BlueprintType)
class AGhostPlacement : public AActor
{
	GENERATED_BODY()

public:
	AGhostPlacement();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProceduralMeshComponent* ProceduralMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GhostStaticMesh;	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Materials")
	UMaterialInstanceDynamic* GhostDynamicMaterialInstanceDynamic;	
	
	virtual void BeginPlay() override;
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* GhostMaterialReference;

	TWeakObjectPtr<AActor> MyOwner;
	
	virtual void Tick(float DeltaTime) override;

private:
	
	
};
