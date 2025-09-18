// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OriginRebasable.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType, Blueprintable)
class UOriginRebasable : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief Interface for components or classes that support origin rebasing functionality.
 *
 * This interface is intended to be inherited by classes that require functionalities
 * for adjusting their internal origin, typically in use cases such as large world coordinates
 * or precision error handling in simulations.
 *
 * Any class implementing this interface should define the necessary logic
 * for rebasing their origin or related operations.
 */
class IOriginRebasable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Origin Rebasing")
	FVector GetActorOriginalLocation();
};
