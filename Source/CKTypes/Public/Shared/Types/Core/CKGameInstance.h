// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Shared/Types/Structures/UserState/FUserState.h"
#include "CKGameInstance.generated.h"




/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class CROWDEDKINGDOMS_API UCKGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	
	virtual void Init() override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Game Instance|Version")
	int32 Major;

	UPROPERTY(BlueprintReadOnly, Category = "Game Instance|Version")
	int32 Minor;

	UPROPERTY(BlueprintReadOnly, Category = "Game Instance|Version")
	int32 Patch;

	UPROPERTY(BlueprintReadOnly, Category = "Game Instance|Version")
	int32 Build;

	UPROPERTY(BlueprintReadWrite, Category = "Game Instance|UserState")
	FUserState UserState;
	
};
