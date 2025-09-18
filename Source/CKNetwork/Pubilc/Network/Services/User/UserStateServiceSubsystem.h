// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "CKTypes/Public/Shared/Types/Structures/UserState/FUserState.h"
#include "UserStateServiceSubsystem.generated.h"

class UGraphQLService;

DECLARE_LOG_CATEGORY_EXTERN(LogUserStateService, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUserStateResponse, bool, bSuccess, FUserState, State);

UCLASS(Blueprintable, BlueprintType)
class UUserStateServiceSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "User State Service Subsystem")
	virtual void PostSubsystemInit() override;

	// User Functions -- Start -- 

	UFUNCTION(BlueprintCallable, Category = "User State Service")
	void GetUserState();

	UFUNCTION(BlueprintCallable, Category = "User State Service")
	void UpdateUserState(const FUserState& State);

	// User Functions -- End --


	// Handler Functions -- Start --

	void HandleGetUserStateResponse(const TSharedPtr<FJsonObject>& Payload) const;
	void HandleUpdateUserStateResponse(const TSharedPtr<FJsonObject>& Payload) const;

	// Handler Functions -- End --



	//Delegate Declaration
	UPROPERTY(BlueprintAssignable, Category = "User State Service")
	FOnUserStateResponse OnUserStateResponse;

private:

	void BroadcastStateResponse(bool bSuccess, FString StateString) const;

	UPROPERTY()
	UGraphQLService* GraphQlService;
};
