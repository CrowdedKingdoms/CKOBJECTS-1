// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameObjects/Framework/Base/GameObjectBase.h"
#include "GO_Light.generated.h"

UCLASS()
class   AGO_Light : public AGameObjectBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGO_Light();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Game Object|Light")
	int32 GetLightOrder();
	
	UPROPERTY(EditAnywhere, Category = "Game Object|Light")
	ULightComponent* Light;
	
	UPROPERTY(EditAnywhere, Category = "Game Object|Light")
	UStaticMeshComponent* LightMesh;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void ActivateGameObject(const FGameObjectState& NewState) override;

	UFUNCTION(BlueprintCallable, Category = "Game Object|Light")
	void TurnOnLight(const FColor NewLightColor, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Game Object|Light")
	void TurnOffLight();
	
	UPROPERTY()
	USceneComponent* RootSceneComponent;

	

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Game Object|Light")
	int32 LightOrder = 0;
};
