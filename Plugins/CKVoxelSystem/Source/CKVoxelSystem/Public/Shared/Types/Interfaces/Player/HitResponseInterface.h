// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HitResponseInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UHitResponseInterface : public UInterface
{
	GENERATED_BODY()
};

class  IHitResponseInterface
{
	GENERATED_BODY()


public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hit Response Handler")
	void HandlePlayerHit(AActor* HitActor, FVector Velocity);
};
