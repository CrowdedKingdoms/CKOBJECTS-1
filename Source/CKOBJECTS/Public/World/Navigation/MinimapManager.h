// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinimapManager.generated.h"

UCLASS(Blueprintable, BlueprintType)
class   AMinimapManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMinimapManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	UFUNCTION(BlueprintCallable, Category = "Minimap Manager")
	FVector2D GetPlayerMinimapPosition2D(const FString PlayerUUID, const FVector OwningPlayerLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Minimap Manager")
	void AddPlayerToMinimap(const FString PlayerUUID, const FVector ActorLocation);

	UFUNCTION(BlueprintCallable, Category = "Minimap Manager")
	void UpdatePlayerLocation(const FString PlayerUUID, const FVector ActorLocation);

	UFUNCTION(BlueprintCallable, Category = "Minimap Manager")
	void RemovePlayerFromMinimap(const FString PlayerUUID);

	UFUNCTION(BlueprintCallable, Category = "Minimap Manager")
	void SetMinimapWidgetReference(UUserWidget* InMinimapWidget){MinimapWidget = InMinimapWidget;}
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap Manager")
	float WorldRadius = 128.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap Manager")
	FVector2D MinimapSize = FVector2D(200, 200);

	
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap Manager")
	TMap<FString, FVector> PlayerLocations;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Minimap Manager")
	UUserWidget* MinimapWidget;
};
