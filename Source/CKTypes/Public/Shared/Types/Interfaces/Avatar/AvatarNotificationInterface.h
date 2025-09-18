// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Structures/Avatar/FAvatarAPIResponses.h"
#include "Shared/Types/Structures/Avatar/FAvatarMetadata.h"
#include "UObject/Interface.h"
#include "AvatarNotificationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UAvatarNotificationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class IAvatarNotificationInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintImplementableEvent, Category = "Avatar Notification")
	void OnAvatarCreated(FCreateAvatarResponse Response);

	UFUNCTION(BlueprintImplementableEvent, Category = "Avatar Notification")
	void OnAvatarsFetched(FGetAvatarsResponse Response);

	UFUNCTION(BlueprintImplementableEvent, Category = "Avatar Notification")
	void OnAvatarNameUpdated(FUpdateAvatarNameResponse Response);

	UFUNCTION(BlueprintImplementableEvent, Category = "Avatar Notification")
	void OnAvatarStateUpdated(FUpdateAvatarStateResponse Response);

	UFUNCTION(BlueprintImplementableEvent, Category = "Avatar Notification")
	void OnAvatarDeleted(FDeleteAvatarResponse Response);
	//~ End Interface Functions
};
