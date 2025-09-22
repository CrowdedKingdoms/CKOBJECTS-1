// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/Infrastructure/QueryMessageParser.h"

void UQueryMessageParser::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	InitializeResponseTypeMap();
}

void UQueryMessageParser::Deinitialize()
{
	Super::Deinitialize();
}

EQueryResponseType UQueryMessageParser::ParseJsonResponse(const FString& JsonString,
                                                          TSharedPtr<FJsonObject>& OutParsedData)
{
	// Create JSON reader
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
	// Parse the JSON
	if (!FJsonSerializer::Deserialize(Reader, OutParsedData) || !OutParsedData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON: %s"), *JsonString);
		return EQueryResponseType::Error;
	}

	// Determine the response type
	return DetermineResponseType(OutParsedData);

}

TSharedPtr<FJsonObject> UQueryMessageParser::GetDataObject(const TSharedPtr<FJsonObject>& ParsedData,
	EQueryResponseType ResponseType)
{
	if (!ParsedData.IsValid())
	{
		return nullptr;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	if (!ParsedData->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		return nullptr;
	}

	// Find the response type name
	FString ResponseTypeName;
	for (const auto& TypePair : ResponseTypeMap)
	{
		if (TypePair.Value == ResponseType)
		{
			ResponseTypeName = TypePair.Key;
			break;
		}
	}

	if (ResponseTypeName.IsEmpty())
	{
		return nullptr;
	}

	const TSharedPtr<FJsonObject>* ResponseObject;
	if ((*DataObject)->TryGetObjectField(ResponseTypeName, ResponseObject))
	{
		return *ResponseObject;
	}

	// Handle array responses (like myAvatars)
	const TArray<TSharedPtr<FJsonValue>>* ResponseArray;
	if ((*DataObject)->TryGetArrayField(ResponseTypeName, ResponseArray))
	{
		// Create a wrapper object for array responses
		TSharedPtr<FJsonObject> WrapperObject = MakeShareable(new FJsonObject);
		WrapperObject->SetArrayField(ResponseTypeName, *ResponseArray);
		return WrapperObject;
	}

	// Handle null responses (like getChunk: null)
	if ((*DataObject)->HasField(ResponseTypeName))
	{
		TSharedPtr<FJsonObject> NullObject = MakeShareable(new FJsonObject);
		NullObject->SetField(ResponseTypeName, (*DataObject)->TryGetField(ResponseTypeName));
		return NullObject;
	}

	return nullptr;

}

bool UQueryMessageParser::HasErrors(const TSharedPtr<FJsonObject>& ParsedData)
{
	if (!ParsedData.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* ErrorsArray;
	return ParsedData->TryGetArrayField(TEXT("errors"), ErrorsArray) && ErrorsArray->Num() > 0;

}

TArray<FString> UQueryMessageParser::GetErrorMessages(const TSharedPtr<FJsonObject>& ParsedData)
{
	TArray<FString> ErrorMessages;
    
	if (!HasErrors(ParsedData))
	{
		return ErrorMessages;
	}

	const TArray<TSharedPtr<FJsonValue>>* ErrorsArray;
	if (ParsedData->TryGetArrayField(TEXT("errors"), ErrorsArray))
	{
		for (const auto& ErrorValue : *ErrorsArray)
		{
			const TSharedPtr<FJsonObject>* ErrorObject;
			if (ErrorValue->TryGetObject(ErrorObject))
			{
				FString Message;
				if ((*ErrorObject)->TryGetStringField(TEXT("message"), Message))
				{
					ErrorMessages.Add(Message);
				}
			}
		}
	}

	return ErrorMessages;

}

TArray<int32> UQueryMessageParser::GetErrorCodes(const TSharedPtr<FJsonObject>& ParsedData)
{
	TArray<int32> ErrorCodes;
    
	if (!HasErrors(ParsedData))
	{
		return ErrorCodes;
	}

	const TArray<TSharedPtr<FJsonValue>>* ErrorsArray;
	if (ParsedData->TryGetArrayField(TEXT("errors"), ErrorsArray))
	{
		for (const auto& ErrorValue : *ErrorsArray)
		{
			const TSharedPtr<FJsonObject>* ErrorObject;
			if (ErrorValue->TryGetObject(ErrorObject))
			{
				int32 Code;
				if ((*ErrorObject)->TryGetNumberField(TEXT("code"), Code))
				{
					ErrorCodes.Add(Code);
				}
			}
		}
	}

	return ErrorCodes;

}

TArray<FString> UQueryMessageParser::GetErrorPaths(const TSharedPtr<FJsonObject>& ParsedData)
{
	TArray<FString> ErrorPaths;
	
	const TArray<TSharedPtr<FJsonValue>>* ErrorsArray;
	if (ParsedData->TryGetArrayField(TEXT("errors"), ErrorsArray))
	{
		for (const auto& ErrorValue : *ErrorsArray)
		{
			const TSharedPtr<FJsonObject>* ErrorObject;
			if (ErrorValue->TryGetObject(ErrorObject))
			{
				const TArray<TSharedPtr<FJsonValue>>* PathArray;
				if ((*ErrorObject)->TryGetArrayField(TEXT("path"), PathArray))
				{
					for (const auto& PathValue : *PathArray)
					{
						FString PathString;
						if (PathValue->TryGetString(PathString))
						{
							ErrorPaths.Add(PathString);
						}
					}
				}
			}
		}
	}
	return ErrorPaths;

}

EQueryResponseType UQueryMessageParser::DetermineErrorResponseType(const TSharedPtr<FJsonObject>& ParsedData)
{
	TArray<FString> ErrorPaths = GetErrorPaths(ParsedData);
    
	if (ErrorPaths.Num() == 0)
	{
		return EQueryResponseType::Error;
	}

	// Get the first path element to determine the operation type
	const FString& Operation = ErrorPaths[0];
    
	// Use your existing ResponseTypeMap to determine the type
	for (const auto& TypePair : ResponseTypeMap)
	{
		if (TypePair.Key == Operation)
		{
			return TypePair.Value;
		}
	}
    
	return EQueryResponseType::Error;

}

EQueryResponseType UQueryMessageParser::DetermineResponseType(const TSharedPtr<FJsonObject>& JsonObject)
{
	// Check for errors first
	if (HasErrors(JsonObject))
	{
		return EQueryResponseType::Error;
	}

	// Check for a data object
	const TSharedPtr<FJsonObject>* DataObject;
	if (!JsonObject->TryGetObjectField(TEXT("data"), DataObject) || !DataObject->IsValid())
	{
		return EQueryResponseType::Error;
	}

	// Check each possible response type in the data object
	for (const auto& TypePair : ResponseTypeMap)
	{
		if ((*DataObject)->HasField(TypePair.Key))
		{
			return TypePair.Value;
		}
	}

	return EQueryResponseType::Error;

}

void UQueryMessageParser::InitializeResponseTypeMap()
{
	ResponseTypeMap.Empty();
	ResponseTypeMap.Add(TEXT("login"), EQueryResponseType::Login);
	ResponseTypeMap.Add(TEXT("register"), EQueryResponseType::Register);
	ResponseTypeMap.Add(TEXT("serverWithLeastClients"), EQueryResponseType::UDP_Info);
	ResponseTypeMap.Add(TEXT("getChunksByDistance"), EQueryResponseType::GetChunkByDistance);
	ResponseTypeMap.Add(TEXT("updateChunk"), EQueryResponseType::UpdateChunk);
	ResponseTypeMap.Add(TEXT("getVoxelList"), EQueryResponseType::VoxelList);
	ResponseTypeMap.Add(TEXT("createAvatar"), EQueryResponseType::CreateAvatar);
	ResponseTypeMap.Add(TEXT("myAvatars"), EQueryResponseType::MyAvatars);
	ResponseTypeMap.Add(TEXT("updateAvatar"), EQueryResponseType::UpdateAvatar);
	ResponseTypeMap.Add(TEXT("updateAvatarState"), EQueryResponseType::UpdateAvatarState);
	ResponseTypeMap.Add(TEXT("teleportRequest"), EQueryResponseType::TeleportRequest);
	ResponseTypeMap.Add(TEXT("updateUserState"), EQueryResponseType::UpdateUserState);
	ResponseTypeMap.Add(TEXT("me"), EQueryResponseType::GetUserState);
	ResponseTypeMap.Add("deleteAvatar", EQueryResponseType::DeleteAvatar);
	ResponseTypeMap.Add("versionInfo", EQueryResponseType::VersionInfo);

}
