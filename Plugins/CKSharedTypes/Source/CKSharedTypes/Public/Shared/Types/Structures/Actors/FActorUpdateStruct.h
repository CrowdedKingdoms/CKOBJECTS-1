﻿#pragma once
#include "CoreMinimal.h"
#include "FActorState.h"
#include "GameFramework/Character.h"
#include "FActorUpdateStruct.generated.h"

USTRUCT(BlueprintType)

struct FActorUpdateStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="ActorState")
	uint8 Version = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	int64 ChunkX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	int64 ChunkY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	int64 ChunkZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	FString UUID;
        
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	FActorState State;
};
