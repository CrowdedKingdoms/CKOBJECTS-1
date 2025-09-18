// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Structures/GameObjects/FGameObjectState.h"
#include "UObject/Interface.h"
#include "Activatable.generated.h"

class AGameObjectBase;

UINTERFACE(MinimalAPI, Blueprintable, BlueprintType)
class UActivatable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Will be used for detection of interactable objects in the game.
 */
class IActivatable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Activatable Interface")
	void ActivateEvent(const FGameObjectState& NewState);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Activatable Interface")
	void RequestActivation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Activatable Interface")
	void RequestActivationWithState(const FGameObjectState& NewState);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Activatable Interface")
	AGameObjectBase* GetAssociatedObject();
};
