// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameObjects/Framework/Base/ActivatorBase.h"
#include "PressurePlate.generated.h"

UCLASS()
class CROWDEDKINGDOMS_API APressurePlate : public AActivatorBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APressurePlate();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, Category = "Pressure Plate")
	USceneComponent* RootSceneComponent;
	
	UPROPERTY(EditAnywhere, Category = "Pressure Plate")
	UStaticMeshComponent* PlateMesh;

	UPROPERTY(EditAnywhere, Category = "Pressure Plate")
	UBoxComponent* BoxComponent;
};
