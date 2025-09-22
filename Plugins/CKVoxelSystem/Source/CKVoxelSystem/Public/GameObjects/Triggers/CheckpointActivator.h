// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameObjects/Framework/Base/ActivatorBase.h"
#include "CheckpointActivator.generated.h"

UCLASS()
class  ACheckpointActivator : public AActivatorBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACheckpointActivator();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	USceneComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, Category = "Checkpoint")
	UBoxComponent* BoxComponent;
};
