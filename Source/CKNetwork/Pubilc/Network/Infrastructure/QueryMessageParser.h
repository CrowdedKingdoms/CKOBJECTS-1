// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Shared/Types/Enums/Network//QueryResponseType.h"
#include "QueryMessageParser.generated.h"



/**
 * UQueryMessageParser is responsible for parsing JSON responses and extracting relevant
 * information such as response type, error messages, and error codes. It provides utility
 * methods for handling and interpreting JSON data based on predefined response types.
 */
UCLASS()
class CROWDEDKINGDOMS_API UQueryMessageParser : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Parse JSON string and return the response type
	EQueryResponseType ParseJsonResponse(const FString& JsonString, TSharedPtr<FJsonObject>& OutParsedData);

	// Get the data object for a specific response type
	TSharedPtr<FJsonObject> GetDataObject(const TSharedPtr<FJsonObject>& ParsedData, EQueryResponseType ResponseType);

	// Check if response contains errors
	bool HasErrors(const TSharedPtr<FJsonObject>& ParsedData);

	// Get error information
	TArray<FString> GetErrorMessages(const TSharedPtr<FJsonObject>& ParsedData);

	// Get error codes
	TArray<int32> GetErrorCodes(const TSharedPtr<FJsonObject>& ParsedData);

	TArray<FString> GetErrorPaths(const TSharedPtr<FJsonObject>& ParsedData);

	EQueryResponseType DetermineErrorResponseType(const TSharedPtr<FJsonObject>& ParsedData);

private:
	// Determine a response type from parsed JSON
	EQueryResponseType DetermineResponseType(const TSharedPtr<FJsonObject>& JsonObject);

	// Map of response type names to enum values
	TMap<FString, EQueryResponseType> ResponseTypeMap;

	// Initialize the response type mapping
	void InitializeResponseTypeMap();

};
