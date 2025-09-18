// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UPlayerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class IPlayerInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player")
	AActor* GetPlayerMinigameZone();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Player")
	USkeletalMeshComponent* GetPlayerMesh();
};
