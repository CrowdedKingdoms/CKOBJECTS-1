#pragma once

#include "CoreMinimal.h"
#include "FOriginOffset.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FOriginOffset
{
	GENERATED_BODY()
	
	UPROPERTY(Blueprintable, BlueprintReadWrite)
	int64 OffsetX;

	UPROPERTY(Blueprintable, BlueprintReadWrite)
	int64 OffsetY;

	UPROPERTY(Blueprintable, BlueprintReadWrite)
	int64 OffsetZ;

	FOriginOffset():
	OffsetX(0), OffsetY(0), OffsetZ(0){};
};