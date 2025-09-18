// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameObjects/Triggers/CheckpointActivator.h"
#include "GameObjects/Framework/Base/GameObjectBase.h"
#include "GO_Checkpoint.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheckpointCrossed, bool, bFinish);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheckpointActivated, AGO_Checkpoint*, Checkpoint);

/**
 * 
 */
UCLASS()
class CROWDEDKINGDOMS_API AGO_Checkpoint : public AGameObjectBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGO_Checkpoint();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintCallable, BlueprintCallable, Category = "Game Object|Checkpoint")
	bool IsFinal() const;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Game Object|Checkpoint")
	FOnCheckpointCrossed OnCheckpointCrossed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Game Object|Checkpoint")
	FOnCheckpointActivated OnCheckpointActivated;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	//float GateWidth = 100.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	//float GateHeight = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	UStaticMesh* PillarMesh;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	UStaticMesh* BannerMesh;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void ActivateGameObject(const FGameObjectState& NewState) override;

	//UPROPERTY(EditAnywhere, Category = "Checkpoint")
	//AGO_Checkpoint* NextCheckpoint;

	//UPROPERTY(EditAnywhere, Category = "Checkpoint")
	//ACheckpointActivator* Activator;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	bool bIsFinalCheckpoint;

private:

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	USceneComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	UStaticMeshComponent* LeftPillar;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	UStaticMeshComponent* RightPillar;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	UStaticMeshComponent* Banner;

	UPROPERTY(EditAnywhere, Category = "Game Object|Checkpoint")
	UBoxComponent* BoxComponent;
};
