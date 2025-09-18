// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MinimapWidgetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UMinimapWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CROWDEDKINGDOMS_API IMinimapWidgetInterface
{
	GENERATED_BODY()

	public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap Widget Interface")
	void AddPlayerToMinimapWidget(const FString& UUID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap Widget Interface")
	void RemovePlayerFromMinimapWidget(const FString& UUID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap Widget Interface")
	void UpdatePlayerLocationOnMinimapWidget();

	
	//~ End Interface Functions
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
