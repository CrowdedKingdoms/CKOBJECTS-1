// Fill out your copyright notice in the Description page of Project Settings.


#include "GameObjects/Triggers/CheckpointActivator.h"

// Sets default values
ACheckpointActivator::ACheckpointActivator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;
	RootSceneComponent->SetMobility(EComponentMobility::Movable);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootSceneComponent);
	BoxComponent->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	BoxComponent->SetCollisionProfileName("OverlapAllDynamic");
	BoxComponent->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void ACheckpointActivator::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACheckpointActivator::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}
