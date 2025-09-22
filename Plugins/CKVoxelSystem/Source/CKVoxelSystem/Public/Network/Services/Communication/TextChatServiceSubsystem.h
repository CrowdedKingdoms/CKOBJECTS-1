// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "Shared/Types/Structures/Services/TextChat/FTextMessage.h"
#include "TextChatServiceSubsystem.generated.h"

/**
 * Handles all logic for Text Chat Services.
 */


class UUDPSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextMessageReceived, const FTextMessage&, ReceivedMessage);


UCLASS(Blueprintable, BlueprintType)
class  UTextChatServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Text Chat Service Subsystem")
	virtual void PostSubsystemInit() override;
	
	// Text Chat Functions
	UFUNCTION(BlueprintCallable, Category= "Text Chat Service")
	void SendTextChatMessage(const FTextMessage& TextMessageToSend);

	void HandleIncomingTextChatMessage(const TArray<uint8>& Payload) const;
	
	UPROPERTY(BlueprintAssignable, Category = "Text Chat Service")
	FOnTextMessageReceived OnTextMessageReceived;

private:

	UPROPERTY()
	UUDPSubsystem* UDPSubsystem;
};
