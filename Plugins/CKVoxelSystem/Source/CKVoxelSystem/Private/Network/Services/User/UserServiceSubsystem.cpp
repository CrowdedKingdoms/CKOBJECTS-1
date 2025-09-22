// Fill out your copyright notice in the Description page of Project Settings.
#include "Network/Services/User/UserServiceSubsystem.h"
#include "Network/GraphQL/GraphQLService.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"

void UUserServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUserServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UUserServiceSubsystem::PostSubsystemInit()
{
	GameSessionSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UGameSessionSubsystem>();
	GraphQlService = GetWorld()->GetGameInstance()->GetSubsystem<UGraphQLService>();

	if (!GameSessionSubsystem || !GraphQlService)
	{
		UE_LOG(LogTemp, Error, TEXT("Some or one of the subsystem is invalid."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("User Service Subsystem Initialized."));
}

// Requests
void UUserServiceSubsystem::Login(FString const Email, const FString Password)
{
	if (!IsValid(GraphQlService))
	{
		UE_LOG(LogTemp, Warning, TEXT("Game Instance is not valid."));
		return;
	}

    UE_LOG(LogTemp, Log, TEXT("CK_DIAG: Login request initiated"));

	TMap<FString, FString> LoginVariables;

	LoginVariables.Add(TEXT("email"), Email);
	LoginVariables.Add(TEXT("password"), Password);

	GraphQlService->ExecuteQueryByID(EGraphQLQuery::Login, LoginVariables, false);
}

void UUserServiceSubsystem::Register(FString const Email, FString const Password)
{
	if (!IsValid(GraphQlService))
	{
		UE_LOG(LogTemp, Warning, TEXT("Game Instance is not valid."));
		return;
	}

    UE_LOG(LogTemp, Log, TEXT("CK_DIAG: Register request initiated"));

	TMap<FString, FString> RegisterVariables;

	RegisterVariables.Add(TEXT("email"), Email);
	RegisterVariables.Add(TEXT("password"), Password);

	GraphQlService->ExecuteQueryByID(EGraphQLQuery::Register, RegisterVariables, false);
}

void UUserServiceSubsystem::Logout()
{
	if (!IsValid(GraphQlService) || !IsValid(GameSessionSubsystem))
	{
		UE_LOG(LogTemp, Warning, TEXT("Game Instance or subsystems are not valid."));
		BroadcastLogoutResponse(false);
		return;
	}

    UE_LOG(LogTemp, Log, TEXT("CK_DIAG: Logout request initiated"));

	// Clear all session data and tokens
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		UE_LOG(LogTemp, Log, TEXT("CK_DIAG: Clearing session data on game thread"));
		
		// Clear GraphQL auth token
		GraphQlService->ClearAuthToken();
		
		// Clear all session data
		GameSessionSubsystem->ClearSessionData();
		
		UE_LOG(LogTemp, Log, TEXT("CK_DIAG: Session data cleared successfully"));
		
		BroadcastLogoutResponse(true);
	});
}

// Handlers 
void UUserServiceSubsystem::HandleLoginResponse(const TSharedPtr<FJsonObject>& Payload) const
{
    UE_LOG(LogTemp, Log, TEXT("CK_DIAG: HandleLoginResponse entered"));
	if (!Payload.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid payload."));
		BroadcastLoginResponse(false, {});
		return;
	}

	FString GameToken;
	int64 UserId = 0;

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* LoginObject;
	const TSharedPtr<FJsonObject>* UserObject;


	// Extract data object first
    if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract data object from response"));
		BroadcastLoginResponse(false, {});
		return;
	}

	// Extract login object from data
    if (!(*DataObject)->TryGetObjectField(TEXT("login"), LoginObject) || !LoginObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract login object from data"));
		BroadcastLoginResponse(false, {});
		return;
	}

	// Extract token from a login object
    if (!(*LoginObject)->TryGetStringField(TEXT("token"), GameToken))
	{
		UE_LOG(LogTemp, Warning, TEXT("Token not found in login object or failed to extract token"));
		BroadcastLoginResponse(false, {});
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Received Login Token: %s"), *GameToken);

	FString GameTokenId_String;

    if (!(*LoginObject)->TryGetStringField(TEXT("gameTokenId"), GameTokenId_String))
	{
		UE_LOG(LogTemp, Warning, TEXT("Token not found in login object or failed to extract token"));
		BroadcastLoginResponse(false, {});
		return;
	}

	int64 GameTokenId;

	if (!GameTokenId_String.IsEmpty())
	{
		GameTokenId = FCString::Atoi64(*GameTokenId_String);
		UE_LOG(LogTemp, Log, TEXT("Game Token Id: %lld"), GameTokenId);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Token not found in login object or failed to extract token"));

		BroadcastLoginResponse(false, {});

		return;
	}


	// Extract user object from the login object
    if ((*LoginObject)->TryGetObjectField(TEXT("user"), UserObject) && UserObject->IsValid())
	{
		FString UserIdString;
		if ((*UserObject)->TryGetStringField(TEXT("userId"), UserIdString))
		{
			UserId = FCString::Atoi64(*UserIdString);
			UE_LOG(LogTemp, Log, TEXT("User ID: %lld"), UserId);
		}
		else
		{
			BroadcastLoginResponse(false, {});


			UE_LOG(LogTemp, Error, TEXT("Failed to extract userId from user object"));
			return;
		}
	}
	else
	{
		BroadcastLoginResponse(false, {});

		UE_LOG(LogTemp, Error, TEXT("Failed to extract user object from login object"));
		return;
	}


    // Ensure game-thread safe state updates in packaged builds
    AsyncTask(ENamedThreads::GameThread, [this, GameToken, GameTokenId, UserId]()
    {
        UE_LOG(LogTemp, Log, TEXT("CK_DIAG: Applying login state on game thread"));
        GameSessionSubsystem->SetUserID(UserId);
        GameSessionSubsystem->SetGameTokenID(GameTokenId);
        GraphQlService->SetAuthToken(GameToken);
        GraphQlService->ExecuteQueryByID(EGraphQLQuery::UDP_Access, TMap<FString, FString>());
    });

	BroadcastLoginResponse(true, GameToken);
}

void UUserServiceSubsystem::HandleRegisterResponse(const TSharedPtr<FJsonObject>& Payload) const
{
    UE_LOG(LogTemp, Log, TEXT("CK_DIAG: HandleRegisterResponse entered"));
	if (!Payload.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid payload."));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnRegisterSuccessful.Broadcast(false, {});
        });

		return;
	}

	FString GameToken;
	int64 UserId = 0;

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* RegisterObject;
	const TSharedPtr<FJsonObject>* UserObject;


	// Extract data object first
	if (!Payload->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract data object from response"));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnRegisterSuccessful.Broadcast(false, {});
        });

		return;
	}

	// Extract login object from data
	if (!(*DataObject)->TryGetObjectField(TEXT("register"), RegisterObject) || !RegisterObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract login object from data"));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnRegisterSuccessful.Broadcast(false, {});
        });
		return;
	}

	// Extract token from a register object
	if (!(*RegisterObject)->TryGetStringField(TEXT("token"), GameToken))
	{
		UE_LOG(LogTemp, Warning, TEXT("Token not found in login object or failed to extract token"));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnRegisterSuccessful.Broadcast(false, {});
        });

		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Received Login Token: %s"), *GameToken);


	// Extract user object from register object
	if ((*RegisterObject)->TryGetObjectField(TEXT("user"), UserObject) && UserObject->IsValid())
	{
		FString UserIdString;
		if ((*UserObject)->TryGetStringField(TEXT("userId"), UserIdString))
		{
			UserId = FCString::Atoi64(*UserIdString);
			UE_LOG(LogTemp, Log, TEXT("User ID: %lld"), UserId);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to extract userId from user object"));
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				OnRegisterSuccessful.Broadcast(false, {});
			}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract user object from login object"));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
		{
			OnRegisterSuccessful.Broadcast(false, {});
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		FFunctionGraphTask::CreateAndDispatchWhenReady([this]
		{
		}, TStatId(), nullptr, ENamedThreads::GameThread);
		return;
	}
    AsyncTask(ENamedThreads::GameThread, [this, GameToken, UserId]()
    {
        GameSessionSubsystem->SetUserID(UserId);
        GraphQlService->SetAuthToken(GameToken);
    });


    AsyncTask(ENamedThreads::GameThread, [this, GameToken]()
    {
        OnRegisterSuccessful.Broadcast(true, GameToken);
    });
}


void UUserServiceSubsystem::BroadcastLoginResponse(bool bSuccess, FString GameToken) const
{
    AsyncTask(ENamedThreads::GameThread, [this, bSuccess, GameToken]()
    {
        OnLoginSuccessful.Broadcast(bSuccess, GameToken);
    });
}

void UUserServiceSubsystem::BroadcastLogoutResponse(bool bSuccess) const
{
    AsyncTask(ENamedThreads::GameThread, [this, bSuccess]()
    {
        OnLogoutSuccessful.Broadcast(bSuccess);
    });
}
