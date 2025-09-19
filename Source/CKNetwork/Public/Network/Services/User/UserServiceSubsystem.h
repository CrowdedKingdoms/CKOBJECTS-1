// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "UserServiceSubsystem.generated.h"

class UGameSessionSubsystem;
class UGraphQLService;


/**
 * Handle logic for all the user services.
 */

// Event Delegates for Handling In-Game
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginSuccessful, bool, bSuccess, FString, GameToken);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRegisterSuccessful, bool, bSuccess, FString, Message);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLogoutSuccessful, bool, bSuccess);

UCLASS(Blueprintable, BlueprintType)
class   UUserServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "User Service Subsystem")
	virtual void PostSubsystemInit() override;
	
	// User Functions -- Start -- 
	
	UFUNCTION(BlueprintCallable, Category = "User Service")
	void Login(FString const Email, const FString Password);

	UFUNCTION(BlueprintCallable, Category = "User Service")
	void Register(FString const Email, FString const Password);
	
	UFUNCTION(BlueprintCallable, Category = "User Service")
	void Logout();
	
	// User Functions -- End --


	// Handler Functions -- Start --

	void HandleLoginResponse(const TSharedPtr<FJsonObject>& Payload) const;
	void HandleRegisterResponse(const TSharedPtr<FJsonObject>& Payload) const;
	
	// Handler Functions -- End --



	//Delegate Declaration
	UPROPERTY(BlueprintAssignable, Category = "User Service")
	FOnLoginSuccessful OnLoginSuccessful;

	UPROPERTY(BlueprintAssignable, Category = "User Service")
	FOnRegisterSuccessful OnRegisterSuccessful;

	UPROPERTY(BlueprintAssignable, Category = "User Service")
	FOnLogoutSuccessful OnLogoutSuccessful;


private:

	static constexpr uint32 OKPayloadLen = 65U;
	static constexpr uint32 BADPayloadLen = 1U;

	void BroadcastLoginResponse(bool bSuccess, FString GameToken) const;
	void BroadcastLogoutResponse(bool bSuccess) const;

	UPROPERTY()
	UGraphQLService* GraphQlService;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;
};
