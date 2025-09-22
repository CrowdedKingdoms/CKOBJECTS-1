#pragma once
#include "CoreMinimal.h"
#include "FColorSlotsSaveData.h"
#include "FAvatarMetadata.generated.h"


USTRUCT(BlueprintType)
struct FAvatarMetadata
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Avatar Metadata")
	uint8 Version;
	
	UPROPERTY(BlueprintReadWrite, Category = "Avatar Metadata")
	int64 ID;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar Metadata")
	FString AvatarName;

	UPROPERTY(BlueprintReadWrite, Category = "Avatar Metadata")
	FColorSlotsSaveData PublicState;
};