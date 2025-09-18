#pragma once
#include "CoreMinimal.h"
#include "FAvatarMetadata.h"
#include "FAvatarAPIResponses.generated.h"


USTRUCT(BlueprintType)
struct FCreateAvatarResponse
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	bool bSuccess;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	int64 AvatarID;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	FString AvatarName;
};

USTRUCT(BlueprintType)
struct FGetAvatarsResponse
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	bool bSuccess;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	TArray<FAvatarMetadata> ParsedAvatars;
};

USTRUCT(BlueprintType)
struct FUpdateAvatarNameResponse
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	bool bSuccess;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	int64 AvatarID;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	FString AvatarName;
};


USTRUCT(BlueprintType)
struct FUpdateAvatarStateResponse
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	bool bSuccess;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	int64 AvatarID;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	FColorSlotsSaveData NewState;
};

USTRUCT(BlueprintType)
struct FDeleteAvatarResponse
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Avatar API Responses")
	bool bSuccess;
};
