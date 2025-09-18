// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AtlasManager.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtlasLoaded, UTexture*, Atlas);

UCLASS(Blueprintable, BlueprintType)
class CROWDEDKINGDOMS_API AAtlasManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAtlasManager();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Atlas Manager")
	void RequestAtlas(int64 AtlasID);

	UFUNCTION(BlueprintCallable, Category = "Atlas Manager")
	bool DoesAtlasExist(int64 AtlasID) const;

	UFUNCTION(BlueprintCallable, Category = "Atlas Manager")
	UTexture* GetAtlas(int64 AtlasID) const;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Atlas Manager")
	FOnAtlasLoaded OnAtlasLoaded;
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Atlas Manager")
	TMap<int64, UTexture*> AtlasMap;
	
};
