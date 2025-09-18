// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Avatar/AvatarDataManager.h"
#include "Blueprint/UserWidget.h"
#include "CKNetwork/Pubilc/Network/GraphQL/GraphQLQuery.h"
#include "CKNetwork/Pubilc/Network/GraphQL/GraphQLService.h"
#include "Shared/Types/Interfaces/Avatar/AvatarNotificationInterface.h"
#include "Shared/Types/Structures/Avatar/FAvatarMetadata.h"
#include "Shared/Types/Structures/Avatar/FColorSlotsSaveData.h"

// Sets default values
AAvatarDataManager::AAvatarDataManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AAvatarDataManager::BeginPlay()
{
	Super::BeginPlay();

	GraphQLService = GetGameInstance()->GetSubsystem<UGraphQLService>();
}


// Called every frame
void AAvatarDataManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAvatarDataManager::CreateAvatar(const FString& AvatarName)
{
	if (!GraphQLService)
	{
		GraphQLService = GetGameInstance()->GetSubsystem<UGraphQLService>();
	}

	// Prepare GraphQL variables
	TMap<FString, FString> Variables;
	Variables.Add(TEXT("name"), AvatarName);

	// Execute the GraphQL mutation
	GraphQLService->ExecuteQueryByID(EGraphQLQuery::CreateAvatar, Variables);
}

void AAvatarDataManager::FetchAvatars()
{
	if (!GraphQLService)
	{
		GraphQLService = GetGameInstance()->GetSubsystem<UGraphQLService>();
	}

	const TMap<FString, FString> Variables; // No variables needed for this query
	GraphQLService->ExecuteQueryByID(EGraphQLQuery::MyAvatars, Variables, true);
}

void AAvatarDataManager::UpdateAvatarName(const int64 AvatarID, const FString& NewName)
{
	if (!GraphQLService)
	{
		GraphQLService = GetGameInstance()->GetSubsystem<UGraphQLService>();
	}

	// Convert AvatarID to string for GraphQL ID scalar type
	const FString AvatarID_Str = LexToString(AvatarID);

	// Prepare GraphQL variables
	TMap<FString, FString> Variables;
	Variables.Add(TEXT("id"), AvatarID_Str);
	Variables.Add(TEXT("name"), NewName);

	// Execute the GraphQL mutation
	GraphQLService->ExecuteQueryByID(EGraphQLQuery::UpdateAvatarName, Variables, true);
}

void AAvatarDataManager::UpdateAvatarState(const int64 AvatarID, const FColorSlotsSaveData NewPublicState)
{
	if (!GraphQLService)
	{
		GraphQLService = GetGameInstance()->GetSubsystem<UGraphQLService>();
	}

	// Convert AvatarID to string for GraphQL ID scalar type
	const FString AvatarID_Str = LexToString(AvatarID);

	// Convert the new public state to Base64 UTF-8 string
	const FString PublicStateString = NewPublicState.ToUTF8String();

	// Prepare GraphQL variables
	TMap<FString, FString> Variables;
	Variables.Add(TEXT("id"), AvatarID_Str);
	Variables.Add(TEXT("publicState"), PublicStateString);

	// Execute the GraphQL mutation
	GraphQLService->ExecuteQueryByID(EGraphQLQuery::UpdateAvatarState, Variables, true);
}

void AAvatarDataManager::DeleteAvatar(const int64 AvatarID)
{
	if (!GraphQLService)
	{
		GraphQLService = GetGameInstance()->GetSubsystem<UGraphQLService>();
	}

	const FString AvatarID_Str = LexToString(AvatarID);
	TMap<FString, FString> Variables; // No variables needed for this query
	Variables.Add(TEXT("id"), AvatarID_Str);

	GraphQLService->ExecuteQueryByID(EGraphQLQuery::DeleteAvatar, Variables, true, false);
}

void AAvatarDataManager::HandleCreateAvatarResponse(const TSharedPtr<FJsonObject>& Response) const
{
	FCreateAvatarResponse CreateAvatarResponse;
	CreateAvatarResponse.AvatarName = "";
	CreateAvatarResponse.AvatarID = 0;
	CreateAvatarResponse.bSuccess = false;

	
	if (!Response.IsValid())
	{
		UE_LOG(LogAvatarService, Error, TEXT("Invalid Avatar response from GraphQL service."));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, CreateAvatarResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarCreated(AvatarCreatorWidget,CreateAvatarResponse);
				},LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		}
		return;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	if (!Response->TryGetObjectField(TEXT("data"), DataObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateAvatar: No 'data' field in response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, CreateAvatarResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarCreated(AvatarCreatorWidget, CreateAvatarResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		}
		return;
	}

	// Check if the "createAvatar" field exists within data
	const TSharedPtr<FJsonObject>* CreateAvatarObject;
	if (!(*DataObject)->TryGetObjectField(TEXT("createAvatar"), CreateAvatarObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateAvatar: No 'createAvatar' field in data"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, CreateAvatarResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarCreated(AvatarCreatorWidget, CreateAvatarResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
				
			}
		}
		return;
	}

	FString AvatarIdString;

	if (!(*CreateAvatarObject)->TryGetStringField(TEXT("avatarId"), AvatarIdString))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateAvatar: Missing avatarId field"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, CreateAvatarResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarCreated(AvatarCreatorWidget,CreateAvatarResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		}
		return;
		
	}

	const int64 AvatarId = FCString::Atoi64(*AvatarIdString);

	
	FString AvatarName;
	if (!(*CreateAvatarObject)->TryGetStringField(TEXT("name"), AvatarName))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateAvatar: Missing name field"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, CreateAvatarResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarCreated(AvatarCreatorWidget,CreateAvatarResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		}
		return;
	}

	
	
	// Avatar ID and fields exist
	UE_LOG(LogTemp, Log, TEXT("CreateAvatar: Avatar created successfully"));

	CreateAvatarResponse.AvatarID = AvatarId;
	CreateAvatarResponse.AvatarName = AvatarName;
	CreateAvatarResponse.bSuccess = true;
	
	if (IsValid(AvatarCreatorWidget))
	{
		if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
		{
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, CreateAvatarResponse]
			{
				IAvatarNotificationInterface::Execute_OnAvatarCreated(AvatarCreatorWidget, CreateAvatarResponse);
			}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			
		}
	}
}

void AAvatarDataManager::HandleFetchAvatarsResponse(const TSharedPtr<FJsonObject>& Response) const
{
	FGetAvatarsResponse GetAvatarsResponse;
	GetAvatarsResponse.bSuccess = false;
	
	
	// Check if Response is valid
	TArray<FAvatarMetadata> ParsedAvatars;
	
	if (!Response.IsValid())
	{
		UE_LOG(LogAvatarService, Warning, TEXT("FetchAvatars: Invalid response received"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, GetAvatarsResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarsFetched(AvatarCreatorWidget, GetAvatarsResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if the "data" field exists
	const TSharedPtr<FJsonObject>* DataObject;
	if (!Response->TryGetObjectField(TEXT("data"), DataObject))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("FetchAvatars: No 'data' field in response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, GetAvatarsResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarsFetched(AvatarCreatorWidget, GetAvatarsResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if "myAvatars" field exists within data
	const TArray<TSharedPtr<FJsonValue>>* MyAvatarsArray;
	if (!(*DataObject)->TryGetArrayField(TEXT("myAvatars"), MyAvatarsArray))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("FetchAvatars: No 'myAvatars' field in data"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, GetAvatarsResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarsFetched(AvatarCreatorWidget, GetAvatarsResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}
	

	for (const TSharedPtr<FJsonValue>& AvatarValue : *MyAvatarsArray)
	{
		const TSharedPtr<FJsonObject>* AvatarObject;
		if (!AvatarValue->TryGetObject(AvatarObject))
		{
			UE_LOG(LogAvatarService, Warning, TEXT("FetchAvatars: Invalid avatar object in array"));
			continue;
		}

		FAvatarMetadata AvatarData;

		// Extract AvatarID (string) and convert to int64
		FString AvatarIdString;
		if ((*AvatarObject)->TryGetStringField(TEXT("avatarId"), AvatarIdString))
		{
			AvatarData.ID = FCString::Atoi64(*AvatarIdString);
		}
		else
		{
			UE_LOG(LogAvatarService, Warning, TEXT("FetchAvatars: Missing avatarId field"));
			continue; // Skip this avatar if no ID
		}

		// Extract Avatar Name (string to FString)
		if (!(*AvatarObject)->TryGetStringField(TEXT("name"), AvatarData.AvatarName))
		{
			UE_LOG(LogAvatarService, Warning, TEXT("FetchAvatars: Missing name field for avatar %lld"), AvatarData.ID);
			AvatarData.AvatarName = TEXT(""); // Set empty string as default
		}

		// Extract Public State and convert from UTF8 string to FColorSlotsSaveData
		FString PublicStateString;
		if ((*AvatarObject)->TryGetStringField(TEXT("publicState"), PublicStateString) && !PublicStateString.IsEmpty())
		{
			AvatarData.PublicState = FColorSlotsSaveData::FromUTF8String(PublicStateString);
		}
		else
		{
			// publicState is null, missing, or empty - PublicState will use the default constructor
			UE_LOG(LogAvatarService, Log, TEXT("FetchAvatars: No publicState for avatar %lld"), AvatarData.ID);
		}

		// Add to parsed avatars array
		ParsedAvatars.Add(AvatarData);

		UE_LOG(LogAvatarService, Log, TEXT("FetchAvatars: Parsed avatar - ID: %lld, Name: %s"),
		       AvatarData.ID, *AvatarData.AvatarName);
	}

	UE_LOG(LogAvatarService, Log, TEXT("FetchAvatars: Successfully parsed %d avatars"), ParsedAvatars.Num());

	GetAvatarsResponse.bSuccess = true;
	GetAvatarsResponse.ParsedAvatars = ParsedAvatars;
	
	// Fire success event
	if (IsValid(AvatarCreatorWidget))
	{
		if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
		{
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, GetAvatarsResponse]
			{
				IAvatarNotificationInterface::Execute_OnAvatarsFetched(AvatarCreatorWidget,GetAvatarsResponse);
			}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			
		}
	};
}

void AAvatarDataManager::HandleUpdateAvatarNameResponse(const TSharedPtr<FJsonObject>& Response) const
{
	FUpdateAvatarNameResponse UpdateAvatarNameResponse;
	UpdateAvatarNameResponse.bSuccess = false;
	UpdateAvatarNameResponse.AvatarName = "";
	UpdateAvatarNameResponse.AvatarID = 0;

	
	FString AvatarName;
	
	// Check if Response is valid
	if (!Response.IsValid())
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarName: Invalid response received"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarNameResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarNameUpdated(AvatarCreatorWidget,UpdateAvatarNameResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if "data" field exists
	const TSharedPtr<FJsonObject>* DataObject;
	if (!Response->TryGetObjectField(TEXT("data"), DataObject))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarName: No 'data' field in response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarNameResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarNameUpdated(AvatarCreatorWidget, UpdateAvatarNameResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if "updateAvatar" field exists within data
	const TSharedPtr<FJsonObject>* UpdateAvatarObject;
	if (!(*DataObject)->TryGetObjectField(TEXT("updateAvatar"), UpdateAvatarObject))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarName: No 'updateAvatar' field in data"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarNameResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarNameUpdated(AvatarCreatorWidget, UpdateAvatarNameResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Extract avatarId
	FString AvatarIdString;
	if (!(*UpdateAvatarObject)->TryGetStringField(TEXT("avatarId"), AvatarIdString) || AvatarIdString.IsEmpty())
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarName: Missing or empty avatarId field"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarNameResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarNameUpdated(AvatarCreatorWidget, UpdateAvatarNameResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Extract name
	
	if (!(*UpdateAvatarObject)->TryGetStringField(TEXT("name"), AvatarName) || AvatarName.IsEmpty())
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarName: Missing or empty name field"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarNameResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarNameUpdated(AvatarCreatorWidget, UpdateAvatarNameResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Both fields exist and are not empty - success
	UE_LOG(LogAvatarService, Log, TEXT("UpdateAvatarName: Avatar name updated successfully - ID: %s, Name: %s"),
	       *AvatarIdString, *AvatarName);

	const int64 AvatarID = FCString::Atoi64(*AvatarIdString); // Convert to int64

	UpdateAvatarNameResponse.AvatarID = AvatarID;
	UpdateAvatarNameResponse.AvatarName = AvatarName;
	UpdateAvatarNameResponse.bSuccess = true;
	
	if (IsValid(AvatarCreatorWidget))
	{
		if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
		{
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarNameResponse]
				{
				IAvatarNotificationInterface::Execute_OnAvatarNameUpdated(AvatarCreatorWidget, UpdateAvatarNameResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		}
	};
}

void AAvatarDataManager::HandleUpdateAvatarStateResponse(const TSharedPtr<FJsonObject>& Response) const
{
	FUpdateAvatarStateResponse UpdateAvatarStateResponse;
	UpdateAvatarStateResponse.bSuccess = false;
	UpdateAvatarStateResponse.AvatarID = 0;
	
	FColorSlotsSaveData DecodedPublicState;
	// Check if Response is valid
	if (!Response.IsValid())
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarState: Invalid response received"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if the "data" field exists
	const TSharedPtr<FJsonObject>* DataObject;
	if (!Response->TryGetObjectField(TEXT("data"), DataObject))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarState: No 'data' field in response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if the "updateAvatarState" field exists within data
	const TSharedPtr<FJsonObject>* UpdateAvatarStateObject;
	if (!(*DataObject)->TryGetObjectField(TEXT("updateAvatarState"), UpdateAvatarStateObject))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarState: No 'updateAvatarState' field in data"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Extract publicState
	FString PublicStateString;
	if (!(*UpdateAvatarStateObject)->TryGetStringField(TEXT("publicState"), PublicStateString))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarState: Missing publicState field"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Check if the base64 string is not empty
	if (PublicStateString.IsEmpty())
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarState: publicState is empty"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	FString AvatarIdString;
	
	if (!(*UpdateAvatarStateObject)->TryGetStringField(TEXT("avatarId"), AvatarIdString))
	{
		UE_LOG(LogAvatarService, Warning, TEXT("UpdateAvatarState: Missing avatarId field"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
			return;
		}
	}

	// Convert to int64
	const int64 AvatarID = FCString::Atoi64(*AvatarIdString); 
	
	// Decode the base64 string using FColorSlotsSaveData::FromUTF8String
	DecodedPublicState = FColorSlotsSaveData::FromUTF8String(PublicStateString);

	// Successfully decoded the public state
	UE_LOG(LogAvatarService, Log, TEXT("UpdateAvatarState: Avatar state updated successfully, publicState decoded"));

	UpdateAvatarStateResponse.AvatarID = AvatarID;
	UpdateAvatarStateResponse.bSuccess = true;
	UpdateAvatarStateResponse.NewState = DecodedPublicState;
	
	if (IsValid(AvatarCreatorWidget))
	{
		if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
		{
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, UpdateAvatarStateResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarStateUpdated(AvatarCreatorWidget, UpdateAvatarStateResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		}
	};
}


void AAvatarDataManager::HandleDeleteAvatarResponse(const TSharedPtr<FJsonObject>& Response) const
{
	// Parse the avatar data
	FDeleteAvatarResponse DeleteResponse;
	DeleteResponse.bSuccess = false;
	
	if (!Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HandleDeleteAvatarResponse: Invalid response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, DeleteResponse]
					{
						IAvatarNotificationInterface::Execute_OnAvatarDeleted(AvatarCreatorWidget, DeleteResponse);
					}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Get the data object
	const TSharedPtr<FJsonObject>* DataObject;
	if (!Response->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HandleDeleteAvatarResponse: No data field in response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, DeleteResponse]
					{
						IAvatarNotificationInterface::Execute_OnAvatarDeleted(AvatarCreatorWidget, DeleteResponse);
					}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	// Get the deleteAvatar object
	const TSharedPtr<FJsonObject>* DeleteAvatarObject;
	if (!(*DataObject)->TryGetObjectField(TEXT("deleteAvatar"), DeleteAvatarObject))
	{
		UE_LOG(LogTemp, Error, TEXT("HandleDeleteAvatarResponse: No deleteAvatar field in data"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, DeleteResponse]
					{
						IAvatarNotificationInterface::Execute_OnAvatarDeleted(AvatarCreatorWidget, DeleteResponse);
					}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
		return;
	}

	
	
	// Extract name (required field in this response)
	FString AvatarName;
	if (!(*DeleteAvatarObject)->TryGetStringField(TEXT("name"), AvatarName))
	{
		UE_LOG(LogTemp, Error, TEXT("HandleDeleteAvatarResponse: Invalid response"));
		if (IsValid(AvatarCreatorWidget))
		{
			if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
			{
				UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, DeleteResponse]
					{
						IAvatarNotificationInterface::Execute_OnAvatarDeleted(AvatarCreatorWidget, DeleteResponse);
					}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
			}
		};
	}
	

	UE_LOG(LogTemp, Log, TEXT("HandleDeleteAvatarResponse: Successfully deleted avatar: %s"), *AvatarName);
	DeleteResponse.bSuccess = true;

	if (IsValid(AvatarCreatorWidget))
	{
		if (AvatarCreatorWidget->Implements<UAvatarNotificationInterface>())
		{
			UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, DeleteResponse]
				{
					IAvatarNotificationInterface::Execute_OnAvatarDeleted(AvatarCreatorWidget, DeleteResponse);
				}, LowLevelTasks::ETaskPriority::BackgroundNormal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
		}
	};
	
}