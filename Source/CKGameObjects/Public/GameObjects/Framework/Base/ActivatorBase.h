// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameObjectBase.h"
#include "Shared/Types/Interfaces/World/OriginRebasable.h"
#include "GameFramework/Actor.h"
#include "Shared/Types/Enums/Events/EEventType.h"
#include "Shared/Types/Interfaces/Interaction/Activatable.h"
#include "ActivatorBase.generated.h"

class AGameObjectsManager;

UCLASS(Blueprintable, BlueprintType)
class   AActivatorBase : public AActor, public IActivatable, public IOriginRebasable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AActivatorBase();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	virtual void ActivateEvent_Implementation(const FGameObjectState& NewState) override;

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	void SetGameObjectManagerReference(AGameObjectsManager* InGameObjectManager);

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	void SetActivatorUUID(const FString& inUUID);

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	void RequestActivation_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	void RequestActivationWithState_Implementation(const FGameObjectState& NewState) override;
	
	UFUNCTION(BlueprintCallable, Category = "Activatable")
	AGameObjectBase* GetAssociatedObject_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Activatable")
	FVector GetActorOriginalLocation_Implementation() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Activatable")
	AGameObjectBase* TargetObject;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Activatable")
	FString ActivatorUUID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Activatable")
	int64 MapID;

	UPROPERTY(EditAnywhere, Category = "Activatable")
	EEventType EventType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Activatable")
	int64 ChunkX;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Activatable")
	int64 ChunkY;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Activatable")
	int64 ChunkZ;

private:

	UPROPERTY()
	AGameObjectsManager* GameObjectsManager;

	UPROPERTY()
	FVector OriginalLocation;
};
