// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shared/Types/Structures/Avatar/FColorSlotsSaveData.h"
#include "AvatarDataManager.generated.h"

class UGraphQLService;
class UUserWidget;

DEFINE_LOG_CATEGORY_STATIC(LogAvatarService, Log, All);

UCLASS(BlueprintType)
class   AAvatarDataManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAvatarDataManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY()
	UGraphQLService* GraphQLService;
	
	UPROPERTY()
	UUserWidget* AvatarCreatorWidget;
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Avatar Data Manager")
	void CreateAvatar(const FString& AvatarName);
	
	UFUNCTION(BlueprintCallable, Category = "Avatar Data Manager")
	void FetchAvatars();

	UFUNCTION(BlueprintCallable, Category = "Avatar Data Manager")
	void UpdateAvatarName(const int64 AvatarID, const FString& NewName);

	UFUNCTION(BlueprintCallable, Category = "Avatar Data Manager")
	void UpdateAvatarState(const int64 AvatarID, const FColorSlotsSaveData NewPublicState);

	UFUNCTION(BlueprintCallable, Category = "Avatar Data Manager")
	void DeleteAvatar(const int64 AvatarID);

	// Handlers 
	void HandleCreateAvatarResponse(const TSharedPtr<FJsonObject>& Response) const;
	void HandleFetchAvatarsResponse(const TSharedPtr<FJsonObject>& Response) const;
	void HandleUpdateAvatarNameResponse(const TSharedPtr<FJsonObject>& Response) const; 
	void HandleUpdateAvatarStateResponse(const TSharedPtr<FJsonObject>& Response) const;
	void HandleDeleteAvatarResponse(const TSharedPtr<FJsonObject>& Response) const;

	UFUNCTION(BlueprintCallable, Category = "Avatar Data Manager")
	void SetAvatarCreatorWidget(UUserWidget* InAvatarCreatorWidget){AvatarCreatorWidget = InAvatarCreatorWidget;}
	
};
