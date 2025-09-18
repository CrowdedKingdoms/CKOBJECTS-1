// Fill out your copyright notice in the Description page of Project Settings.
#include "CKNetwork/Pubilc/Network/Services/User/UserStateServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/User/UserServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/GraphQL/GraphQLService.h"

DEFINE_LOG_CATEGORY(LogUserStateService);

void UUserStateServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUserStateServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UUserStateServiceSubsystem::PostSubsystemInit()
{
	GraphQlService = GetWorld()->GetGameInstance()->GetSubsystem<UGraphQLService>();

	if (!GraphQlService)
	{
		UE_LOG(LogUserStateService, Error, TEXT("Some or one of the subsystem is invalid."));
		return;
	}

	UE_LOG(LogUserStateService, Log, TEXT("User State Service Subsystem Initialized."));
}

void UUserStateServiceSubsystem::GetUserState()
{
	if (!IsValid(GraphQlService))
	{
		UE_LOG(LogUserStateService, Warning, TEXT("Game Instance is not valid."));
		return;
	}

	TMap<FString, FString> Variables;

	GraphQlService->ExecuteQueryByID(EGraphQLQuery::GetUserState, Variables, true, false);
}

void UUserStateServiceSubsystem::UpdateUserState(const FUserState& State)
{
	if (!IsValid(GraphQlService))
	{
		UE_LOG(LogUserStateService, Warning, TEXT("Game Instance is not valid."));
		return;
	}

	TMap<FString, FString> Variables;
	Variables.Add(TEXT("stateStr"), State.ToUTF8String());

	GraphQlService->ExecuteQueryByID(EGraphQLQuery::UpdateUserState, Variables, true, false);
}

void UUserStateServiceSubsystem::HandleGetUserStateResponse(const TSharedPtr<FJsonObject>& Payload) const
{
	if (!Payload.IsValid())
	{
		UE_LOG(LogUserStateService, Warning, TEXT("Invalid payload."));
		BroadcastStateResponse(false, {});
		return;
	}

	FString UserState;
	int64 UserId = 0;

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* MeObject;

	// Extract data object first
	if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogUserStateService, Error, TEXT("Failed to extract data object from response"));
		BroadcastStateResponse(false, {});
		return;
	}

	// Extract login object from data
	if (!(*DataObject)->TryGetObjectField(TEXT("me"), MeObject) || !MeObject->IsValid())
	{
		UE_LOG(LogUserStateService, Error, TEXT("Failed to extract user object from data"));
		BroadcastStateResponse(false, {});
		return;
	}

	// Extract token from a login object
	if (!(*MeObject)->TryGetStringField(TEXT("state"), UserState))
	{
		UE_LOG(LogUserStateService, Warning, TEXT("State not found in user object or failed to extract state"));
		BroadcastStateResponse(false, {});
		return;
	}

	UE_LOG(LogUserStateService, Log, TEXT("Received user state: %s"), *UserState);

	FString UserIdString;
	if ((*MeObject)->TryGetStringField(TEXT("userId"), UserIdString))
	{
		UserId = FCString::Atoi64(*UserIdString);
		UE_LOG(LogUserStateService, Log, TEXT("User ID: %lld"), UserId);
	}

	BroadcastStateResponse(true, UserState);
}

void UUserStateServiceSubsystem::HandleUpdateUserStateResponse(const TSharedPtr<FJsonObject>& Payload) const
{
	if (!Payload.IsValid())
	{
		UE_LOG(LogUserStateService, Warning, TEXT("Invalid payload."));
		return;
	}

	int64 UserId = 0;

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* UpdateUserStateObject;

	// Extract data object first
	if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogUserStateService, Error, TEXT("Failed to extract data object from response"));
		return;
	}

	// Extract login object from data
	if (!(*DataObject)->TryGetObjectField(TEXT("updateUserState"), UpdateUserStateObject) || !UpdateUserStateObject->IsValid())
	{
		UE_LOG(LogUserStateService, Error, TEXT("Failed to extract user object from data"));
		return;
	}

	FString UserIdString;
	if ((*UpdateUserStateObject)->TryGetStringField(TEXT("userId"), UserIdString))
	{
		UserId = FCString::Atoi64(*UserIdString);
		UE_LOG(LogUserStateService, Log, TEXT("User ID: %lld"), UserId);
	}
	else
	{
		UE_LOG(LogUserStateService, Error, TEXT("Failed to extract userId from user object"));
	}
}

void UUserStateServiceSubsystem::BroadcastStateResponse(bool bSuccess, FString StateString) const
{
	FUserState State = FUserState::FromUTF8String(StateString);

	//UE_LOG(LogUserStateService, Log, TEXT("UserState response received and broadcast"));
	//OnUserStateResponse.Broadcast(bSuccess, State);

	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, bSuccess, State]()
		{
			UE_LOG(LogUserStateService, Log, TEXT("UserState response received and broadcast"));
			OnUserStateResponse.Broadcast(bSuccess, State);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}