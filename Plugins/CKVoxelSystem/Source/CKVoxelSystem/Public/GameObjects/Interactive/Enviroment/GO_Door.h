// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameObjects/Framework/Base/GameObjectBase.h"
#include "GO_Door.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDoorClosed);

UCLASS()
class  AGO_Door : public AGameObjectBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGO_Door();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Game Object|Door")
	FOnDoorClosed OnDoorClosed;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Game Object|Door")
	void OpenDoorWithTimeline(float& Value) const;

	UFUNCTION(BlueprintCallable, Category = "Game Object|Door")
	void DoorOpened();

	UFUNCTION(BlueprintCallable, Category = "Game Object|Door")
	void CloseDoor();

	virtual void ActivateGameObject(const FGameObjectState& NewState) override;
	
	UPROPERTY(EditAnywhere, Category = "Game Object|Door")
	USceneComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, Category = "Game Object|Door")
	UInstancedStaticMeshComponent* DoorFrameMesh;

	UPROPERTY(EditAnywhere, Category = "Game Object|Door")
	UStaticMeshComponent* DoorMesh;

	UPROPERTY(EditAnywhere, Category = "Game Object|Door|Timeline")
	FTimeline DoorTimeline;

	UPROPERTY(EditAnywhere, Category = "Game Object|Door|Timeline")
	UCurveFloat* DoorTimelineCurve;
	
	UPROPERTY(EditAnywhere, Category = "Game Object|Door|Rotation")
	FRotator DoorStartingRotation;

	UPROPERTY(EditAnywhere, Category = "Game Object|Door|Rotation")
	FRotator DoorFinalRotation;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Game Object|Door|Rotation")
	bool bResetDoorAfterActivation;
};
