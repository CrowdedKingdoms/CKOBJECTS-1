// Fill out your copyright notice in the Description page of Project Settings.


#include "CKNetwork/Pubilc/Network/GraphQL/GraphQLService.h"
#include "HttpModule.h"
#include "CKPlayer/Public/Player/Avatar//AvatarDataManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "CKNetwork/Pubilc/Network/Infrastructure/QueryMessageParser.h"
#include "Shared/Types/Enums/Network/QueryResponseType.h"
#include "CKNetwork/Pubilc/Network/Services/User/UserServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/User/UserStateServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/ChunkServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/VoxelServiceSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/Core/UDPSubsystem.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY(LogGraphQLService);

void UGraphQLService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default to development environment
	bIsDevelopment = true;


	GraphQLQueryDatabase = LoadObject<UGraphQLQueryDatabase>(
		nullptr, TEXT("/Game/Core/GraphQL/DA_Queries.DA_Queries"));

	if (!IsValid(GraphQLQueryDatabase))
	{
		UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Query Database is invalid."));
	}

	UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service initialized"));
}

void UGraphQLService::Deinitialize()
{
	// Clear any stored tokens
	ClearAuthToken();
	MessageParser = nullptr;
	Super::Deinitialize();

	UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service deinitialized"));
}

void UGraphQLService::PostSubsystemInit()
{
	UserService = GetWorld()->GetGameInstance()->GetSubsystem<UUserServiceSubsystem>();
	UserStateService = GetWorld()->GetGameInstance()->GetSubsystem<UUserStateServiceSubsystem>();
	ChunkService = GetWorld()->GetGameInstance()->GetSubsystem<UChunkServiceSubsystem>();
	VoxelService = GetWorld()->GetGameInstance()->GetSubsystem<UVoxelServiceSubsystem>();
	UDP_Service = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();
	MessageParser = GetWorld()->GetGameInstance()->GetSubsystem<UQueryMessageParser>();

	if (!UserService || !ChunkService || !VoxelService || !UDP_Service || !UserStateService)
	{
		UE_LOG(LogGraphQLService, Error, TEXT("Some or one of the subsystems is invalid."));
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(StatsTimerHandle, this, &UGraphQLService::UpdateStats, 1.0f, true);

	UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service Subsystem Initialized."));
}

void UGraphQLService::SetAuthToken(const FString& InAuthToken)
{
	AuthToken = InAuthToken;
	UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service: Auth token set %s"), *AuthToken);
}

void UGraphQLService::ClearAuthToken()
{
	AuthToken.Empty();
	UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service: Auth token cleared"));
}

bool UGraphQLService::HasAuthToken() const
{
	return !AuthToken.IsEmpty();
}

void UGraphQLService::SetEndpoint(const FString& InEndpoint)
{
	GraphQLEndpoint = InEndpoint;
}

FString UGraphQLService::GetCurrentEndpoint() const
{
	return GraphQLEndpoint;
}

FGraphQLStats UGraphQLService::GetStats() const
{
	FGraphQLStats Stats;
	Stats.QueriesSentPerSecond = QueriesSentPerSecond.GetValue();
	Stats.QueryBytesSentPerSecond = QueriesBytesSentPerSecond.GetValue();
	Stats.ResponseBytesReceivedPerSecond = ResponseBytesReceivedPerSecond.GetValue();
	Stats.ResponseReceivedPerSecond = ResponseReceivedPerSecond.GetValue();
	Stats.bIsReceivingData = bIsReceivingData;

	return Stats;
}

void UGraphQLService::ExecuteGraphQLQuery(const FString& Query, const bool bIncludeAuthToken,
                                          const TSharedPtr<FJsonObject>& Variables)
{
	if (Query.IsEmpty())
	{
		UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: Empty query provided"));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
		{
			OnComplete.ExecuteIfBound(false, TEXT("{\"error\": \"Empty query provided\"}"));
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);

		return;
	}

	// Get HTTP module
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http)
	{
		UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: Failed to get HTTP module"));
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
		{
			OnComplete.ExecuteIfBound(false, TEXT("{\"error\": \"HTTP module not available\"}"));
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);

		return;
	}

    // Create HTTP request
	const TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->SetURL(GetCurrentEndpoint());
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetTimeout(30.0f);

	// Add an authorization header only if requested and a token is available
	if (bIncludeAuthToken && HasAuthToken())
	{
		const FString AuthHeader = FString::Printf(TEXT("Bearer %s"), *AuthToken);
		Request->SetHeader(TEXT("Authorization"), AuthHeader);
		UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service: Added authorization header"));
	}
	else if (bIncludeAuthToken && !HasAuthToken())
	{
		UE_LOG(LogGraphQLService, Warning, TEXT("GraphQL Service: Auth requested but no token available"));
	}
	else
	{
		UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service: Sending request without authorization"));
	}

	// Create JSON payload
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("query"), Query);

	if (Variables.IsValid())
	{
		JsonObject->SetObjectField(TEXT("variables"), Variables);
	}

	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(OutputString);

    // Bind response delegate synchronously
    Request->OnProcessRequestComplete().BindUObject(this, &UGraphQLService::OnHttpsRequestComplete);


	// Execute request
    if (!Request->ProcessRequest())
	{
		UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: Failed to process HTTP request"));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnComplete.ExecuteIfBound(false, TEXT("{\"error\": \"Failed to process HTTP request\"}"));
        });
	}
	else
	{
		// Stats
		QueriesSentPerSecond.Increment();
		int32 QuerySize = 0;
		QuerySize = Request->GetVerb().Len();

		TArray<FString> Headers = Request->GetAllHeaders();
		for (const FString& Header : Headers)
		{
			QuerySize += Header.Len();
		}

		QuerySize += Request->GetContentLength();
		QueriesBytesSentPerSecond.Set(QueriesBytesSentPerSecond.GetValue() + QuerySize);

		UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service: GraphQL query sent to %s"), *GetCurrentEndpoint());
	}
}

void UGraphQLService::ExecuteQueryByID(EGraphQLQuery QueryID, const TMap<FString, FString>& RuntimeVariables,
                                       const bool bIncludeAuthToken, const bool bUseNestedJson)
{
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, QueryID, RuntimeVariables, bIncludeAuthToken, bUseNestedJson]()
	{
		if (!GraphQLQueryDatabase)
		{
			UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: QueryDatabase is null"));
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				OnComplete.ExecuteIfBound(false, TEXT("{\"error\": \"Internal error: query database missing\"}"));
			}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			return;
		}

		FGraphQLQueryDef QueryDef;
		if (!GraphQLQueryDatabase->GetQueryByID(QueryID, QueryDef))
		{
			UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: Query not found for ID %d"), static_cast<int32>(QueryID));
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this]()
			{
				OnComplete.ExecuteIfBound(false, TEXT("{\"error\": \"Query not found\"}"));
			}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			return;
		}

		// Merge variables
		const TSharedPtr<FJsonObject> FinalVariables = MakeShareable(new FJsonObject);

		// Start with default vars from the asset
		for (const auto& Pair : QueryDef.DefaultVariables)
		{
			FinalVariables->SetStringField(Pair.Key, Pair.Value);
		}

		if (!bUseNestedJson)
		{
			// Overwrite/append with runtime vars
			for (const auto& Pair : RuntimeVariables)
			{
				FinalVariables->SetStringField(Pair.Key, Pair.Value);
			}
		}
		else
		{
			for (const auto& Pair : RuntimeVariables)
			{
				const FString& DotKey = Pair.Key;
				const FString& Value = Pair.Value;

				TArray<FString> Keys;
				DotKey.ParseIntoArray(Keys, TEXT("."));

				TSharedPtr<FJsonObject> Current = FinalVariables;

				for (int32 i = 0; i < Keys.Num(); ++i)
				{
					const FString& Key = Keys[i];

					if (i == Keys.Num() - 1)
					{
						// Last key: set the value as number or string
						if (Value.IsNumeric())
						{
							// Try parse as int first, fallback to double if needed
							if (Value.IsNumeric() && !Value.Contains(TEXT(".")))
							{
								int64 IntVal = FCString::Atoi64(*Value);
								Current->SetNumberField(Key, static_cast<double>(IntVal));
							}
							else
							{
								double DoubleVal = FCString::Atod(*Value);
								Current->SetNumberField(Key, DoubleVal);
							}
						}
						else
						{
							Current->SetStringField(Key, Value);
						}
					}
					else
					{
						// Intermediate keys: descend or create nested object
						TSharedPtr<FJsonObject> Next;
						const TSharedPtr<FJsonObject>* ExistingPtr = nullptr;
						if (Current->TryGetObjectField(Key, ExistingPtr) && ExistingPtr && ExistingPtr->IsValid())
						{
							Next = *ExistingPtr;
						}
						else
						{
							Next = MakeShared<FJsonObject>();
							Current->SetObjectField(Key, Next);
						}
						Current = Next;
					}
				}
			}
		}

		// Delegate to the existing ExecuteGraphQLQuery
		ExecuteGraphQLQuery(QueryDef.QueryBody, bIncludeAuthToken, FinalVariables);
	}, LowLevelTasks::ETaskPriority::BackgroundNormal);
}

void UGraphQLService::OnHttpsRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, bWasSuccessful, Response]()
	{
		if (!bWasSuccessful || !Response.IsValid())
		{
			UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: HTTP request failed"));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnComplete.ExecuteIfBound(false, TEXT("{\"error\": \"HTTP request failed\"}"));
        });

			return;
		}

        const int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        // Dispatch all follow-up work to the game thread for safety in packaged builds
        AsyncTask(ENamedThreads::GameThread, [this, ResponseCode, ResponseContent]() mutable
        {
            UE_LOG(LogGraphQLService, Log, TEXT("GraphQL Service: Response code: %d"), ResponseCode);
            UE_LOG(LogGraphQLService, Verbose, TEXT("GraphQL Service: Response content length: %d"), ResponseContent.Len());

            const bool bSuccess = (ResponseCode >= 200 && ResponseCode < 300);

            if (!bSuccess)
            {
                UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: Server returned error code: %d"), ResponseCode);
                if (ResponseContent.IsEmpty())
                {
                    ResponseContent = FString::Printf(TEXT("{\"error\": \"Server returned status code %d\"}"), ResponseCode);
                }
                OnComplete.ExecuteIfBound(false, ResponseContent);
                return;
            }

            ResponseReceivedPerSecond.Increment();
            ResponseBytesReceivedPerSecond.Set(ResponseBytesReceivedPerSecond.GetValue() + ResponseContent.Len());
        	bIsReceivingData = true;

            // Route to services on game thread
            ParseAndDispatchToServices(ResponseContent);

        	
        	
            OnComplete.ExecuteIfBound(true, ResponseContent);
        });
    });
}

void UGraphQLService::ParseAndDispatchToServices(const FString& ResponseContent) const
{
	TSharedPtr<FJsonObject> ParsedData;
	const EQueryResponseType ResponseType = MessageParser->ParseJsonResponse(ResponseContent, ParsedData);

	// Handler Errors first if any
	if (ResponseType == EQueryResponseType::Error)
	{
		TArray<FString> ErrorMessages = MessageParser->GetErrorMessages(ParsedData);
		TArray<int32> ErrorCodes = MessageParser->GetErrorCodes(ParsedData);
		TArray<FString> ErrorPaths = MessageParser->GetErrorPaths(ParsedData);

		const EQueryResponseType ErrorResponseType = MessageParser->DetermineErrorResponseType(ParsedData);

		UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Service: Error response received"));

		UE_LOG(LogGraphQLService, Log, TEXT("Handling error for operation type: %d"), static_cast<int32>(ErrorResponseType));

		// Log error details
		for (int32 i = 0; i < ErrorMessages.Num(); ++i)
		{
			FString ErrorMessage = ErrorMessages[i];
			const int32 ErrorCode = (i < ErrorCodes.Num()) ? ErrorCodes[i] : 0;
			FString ErrorPath = (i < ErrorPaths.Num()) ? ErrorPaths[i] : TEXT("unknown");

			UE_LOG(LogGraphQLService, Error, TEXT("GraphQL Error - Path: %s, Code: %d, Message: %s"),
			       *ErrorPath, ErrorCode, *ErrorMessage);
		}

		// Dispatch to the appropriate service based on an error type
		switch (ErrorResponseType)
		{
		case EQueryResponseType::Login: UserService->HandleLoginResponse(ParsedData);
			break;
		case EQueryResponseType::Register: UserService->HandleRegisterResponse(ParsedData);
			break;
		case EQueryResponseType::UDP_Info: UDP_Service->HandleUDPAddressNotification(ParsedData);
			break;
		case EQueryResponseType::UpdateChunk: break;
		case EQueryResponseType::GetChunkByDistance: break;
		case EQueryResponseType::CreateAvatar: break;
		case EQueryResponseType::MyAvatars: break;
		case EQueryResponseType::UpdateAvatar: break;
		case EQueryResponseType::UpdateAvatarState: break;
		case EQueryResponseType::TeleportRequest: 
			HandleTeleportResponse(ParsedData); 
			break;
		case EQueryResponseType::UpdateUserState:
			UE_LOG(LogGraphQLService, Error, TEXT("UpdateUserState error reponse received!"));
			UserStateService->HandleUpdateUserStateResponse(ParsedData);
			break;
		case EQueryResponseType::GetUserState:
			UE_LOG(LogGraphQLService, Error, TEXT("GetUserState error reponse received!"));
			UserStateService->HandleGetUserStateResponse(ParsedData);
			break;
		default:
			UE_LOG(LogGraphQLService, Error, TEXT("Unknown error response type: %d"), static_cast<int32>(ErrorResponseType));
			break;
		}

		return; // Exit early since we handled the error
	}

	switch (ResponseType)
	{
	case Login:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling login response"));
		UserService->HandleLoginResponse(ParsedData);
		break;

	case EQueryResponseType::Register:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling register response"));
		UserService->HandleRegisterResponse(ParsedData);
		break;

	case UDP_Info:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling UDP info response"));
		UDP_Service->HandleUDPAddressNotification(ParsedData);
		break;

	case GetChunkByDistance:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling get chunk response"));
		ChunkService->HandleGetChunkResponse(ParsedData);
		break;

	case UpdateChunk:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling update chunk response"));
		ChunkService->HandleUpdateChunkResponse(ParsedData);
		break;

	case VoxelList:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling voxel list response"));
		VoxelService->HandleVoxelListGraphQLResponse(ParsedData);
		break;

	case CreateAvatar:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling create avatar response"));
		AvatarDataManager->HandleCreateAvatarResponse(ParsedData);
		break;

	case MyAvatars:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling my avatars response"));
		AvatarDataManager->HandleFetchAvatarsResponse(ParsedData);
		break;

	case UpdateAvatar:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling update avatar response"));
		AvatarDataManager->HandleUpdateAvatarNameResponse(ParsedData);
		break;

	case UpdateAvatarState:
		UE_LOG(LogGraphQLService, Log, TEXT("Handling update avatar state response"));
		AvatarDataManager->HandleUpdateAvatarStateResponse(ParsedData);
		break;

	case TeleportRequest:
		HandleTeleportResponse(ParsedData);
		break;

	case UpdateUserState:
		UE_LOG(LogGraphQLService, Log, TEXT("UpdateUserState reponse received!"));
		UserStateService->HandleUpdateUserStateResponse(ParsedData);
		break;

	case GetUserState:
		UE_LOG(LogGraphQLService, Log, TEXT("GetUserState reponse received!"));
		UserStateService->HandleGetUserStateResponse(ParsedData);
		break;

	case DeleteAvatar:
		AvatarDataManager->HandleDeleteAvatarResponse(ParsedData);
		break;

	case VersionInfo:
		HandleGetVersionInfo(ParsedData);
		break;
		
	case Error:
		UE_LOG(LogGraphQLService, Error, TEXT("Error response type"));
		break;

	default:
		UE_LOG(LogGraphQLService, Error, TEXT("Unknown response type"));
		break;
	}
}

void UGraphQLService::HandleTeleportResponse(const TSharedPtr<FJsonObject>& Response) const
{
	UE_LOG(LogGraphQLService, Log, TEXT("Handling teleport request response"));

	if (!Response.IsValid())
	{
		UE_LOG(LogGraphQLService, Warning, TEXT("Invalid Teleport response"));
		return;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	const TSharedPtr<FJsonObject>* TeleportRequestObject;

	// Extract data object first
	if (!Response->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogGraphQLService, Error, TEXT("Failed to extract data object from teleport response"));
		return;
	}

	// Extract teleport response object
	if (!(*DataObject)->TryGetObjectField(TEXT("teleportRequest"), TeleportRequestObject) || !TeleportRequestObject->IsValid())
	{
		UE_LOG(LogGraphQLService, Error, TEXT("Failed to extract teleportRequest from data object"));
		return;
	}

	bool TeleportAllowed = (*TeleportRequestObject)->TryGetField(TEXT("success"))->AsBool();

	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, TeleportAllowed]()
		{
			OnTeleport.Broadcast(TeleportAllowed);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}

void UGraphQLService::HandleGetVersionInfo(const TSharedPtr<FJsonObject>& Response) const
{
	UE_LOG(LogTemp, Log, TEXT("Handling get version info response"));

	if (!Response.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid GetVersionInfo response"));
		return;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	if (!Response->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract data object from version info response"));
		return;
	}

	const TSharedPtr<FJsonObject>* VersionInfoObject;
	if (!(*DataObject)->TryGetObjectField(TEXT("versionInfo"), VersionInfoObject) || !VersionInfoObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extract versionInfo from data object"));
		return;
	}

	// Parse server version
	FGameVersion ServerVersion;
	const TSharedPtr<FJsonObject>* ServerVersionObject;
	if ((*VersionInfoObject)->TryGetObjectField(TEXT("serverVersion"), ServerVersionObject) && ServerVersionObject->IsValid())
	{
		ServerVersion.MajorVersion = (*ServerVersionObject)->GetIntegerField(TEXT("major"));
		ServerVersion.MinorVersion = (*ServerVersionObject)->GetIntegerField(TEXT("minor"));
		ServerVersion.PatchVersion = (*ServerVersionObject)->GetIntegerField(TEXT("patch"));
		ServerVersion.BuildNumber = (*ServerVersionObject)->GetIntegerField(TEXT("build"));
	}

	// Parse minimum client version
	FGameVersion MinimumClientVersion;
	const TSharedPtr<FJsonObject>* MinClientVersionObject;
	if ((*VersionInfoObject)->TryGetObjectField(TEXT("minimumClientVersion"), MinClientVersionObject) && MinClientVersionObject->IsValid())
	{
		MinimumClientVersion.MajorVersion = (*MinClientVersionObject)->GetIntegerField(TEXT("major"));
		MinimumClientVersion.MinorVersion = (*MinClientVersionObject)->GetIntegerField(TEXT("minor"));
		MinimumClientVersion.PatchVersion = (*MinClientVersionObject)->GetIntegerField(TEXT("patch"));
		MinimumClientVersion.BuildNumber = (*MinClientVersionObject)->GetIntegerField(TEXT("build"));
	}

	UE_LOG(LogTemp, Log, TEXT("Server Version: v%d.%d.%d.%d"), 
		ServerVersion.MajorVersion, ServerVersion.MinorVersion, 
		ServerVersion.PatchVersion, ServerVersion.BuildNumber);

	UE_LOG(LogTemp, Log, TEXT("Minimum Client Version: v%d.%d.%d.%d"), 
		MinimumClientVersion.MajorVersion, MinimumClientVersion.MinorVersion, 
		MinimumClientVersion.PatchVersion, MinimumClientVersion.BuildNumber);

	// Broadcast the server version info (you can modify this to broadcast both versions if needed)
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, ServerVersion, MinimumClientVersion]()
	{
		OnGetVersionInfo.Broadcast(ServerVersion, MinimumClientVersion);
	}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);

}


void UGraphQLService::UpdateStats()
{
	QueriesSentPerSecond.Set(0);
	QueriesBytesSentPerSecond.Set(0);
	ResponseBytesReceivedPerSecond.Set(0);
	ResponseReceivedPerSecond.Set(0);
	bIsReceivingData = false;
}
