// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CKTypes/Public/Shared/Types/Interfaces/World/OriginRebasable.h"
#include "CKTypes/Public/Shared/Types/Structures/GameObjects/FGameObjectState.h"
#include "GameObjectBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class AGameObjectBase : public AActor, public IOriginRebasable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameObjectBase();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "GameObjectBase|Activation")
	virtual void ActivateGameObject(const FGameObjectState& NewState);

	UFUNCTION(BlueprintCallable, Category = "GameObjectBase|Activation")
	FGameObjectState& GetGameObjectState();

	UFUNCTION(BlueprintCallable, Category = "GameObjectBase|Origin Rebase")

	FVector GetActorOriginalLocation_Implementation() override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "GameObjectBase|State")
	virtual void SetGameObjectState(const FGameObjectState& NewState);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "GameObjectBase|State")
	FGameObjectState State;

	UPROPERTY()
	FVector OriginalLocation;
};

