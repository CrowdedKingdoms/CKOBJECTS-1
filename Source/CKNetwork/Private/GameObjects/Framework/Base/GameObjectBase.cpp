// Fill out your copyright notice in the Description page of Project Settings.

#include "GameObjects/Framework/Base/GameObjectBase.h"


// Sets default values
AGameObjectBase::AGameObjectBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AGameObjectBase::BeginPlay()
{
	Super::BeginPlay();
	OriginalLocation = GetActorLocation();
}

void AGameObjectBase::SetGameObjectState(const FGameObjectState& NewState)
{
	UE_LOG(LogTemp, Log, TEXT("GameObjectBaseObject: Set State"));
	State = NewState;
	UE_LOG(LogTemp, Log, TEXT("New State set to: %s"), State.bIsActive ? TEXT("Active") : TEXT("Inactive"))
}

// Called every frame
void AGameObjectBase::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameObjectBase::ActivateGameObject(const FGameObjectState& NewState)
{
	SetGameObjectState(NewState);
	//TODO: Implement all other functionality here
	UE_LOG(LogTemp, Log, TEXT("GameObjectBaseObject: Activated"));
}

FGameObjectState& AGameObjectBase::GetGameObjectState()
{
	return State;
}

FVector AGameObjectBase::GetActorOriginalLocation_Implementation()
{
	return OriginalLocation;
}
