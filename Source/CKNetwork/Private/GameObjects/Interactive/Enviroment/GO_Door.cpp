// Fill out your copyright notice in the Description page of Project Settings.

#include "GameObjects/Interactive/Enviroment/GO_Door.h"
#include "Components/InstancedStaticMeshComponent.h"


// Sets default values
AGO_Door::AGO_Door()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	DoorFrameMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DoorFrameMesh"));
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorFrameMesh->SetupAttachment(RootSceneComponent);
	DoorMesh->SetupAttachment(RootSceneComponent);

	DoorMesh->SetRelativeRotation(DoorStartingRotation);

	PrimaryActorTick.SetTickFunctionEnable(false);
}

// Called when the game starts or when spawned
void AGO_Door::BeginPlay()
{
	Super::BeginPlay();

	if (DoorTimelineCurve)
	{
		FOnTimelineFloat TimelineCallback;
		TimelineCallback.BindUFunction(this, FName("OpenDoorWithTimeline"));
		
		FOnTimelineVector TimelineFinishedCallback;
		TimelineFinishedCallback.BindUFunction(this, FName("DoorOpened"));

		DoorTimeline.AddInterpFloat(DoorTimelineCurve, TimelineCallback);
		DoorTimeline.SetTimelineFinishedFunc(static_cast<FOnTimelineEvent>(TimelineFinishedCallback));

		DoorTimeline.SetLooping(false);
	}
}

void AGO_Door::OpenDoorWithTimeline(float& Value) const
{
	const FRotator NewRotation = FMath::Lerp(DoorStartingRotation, DoorFinalRotation, Value);
	DoorMesh->SetRelativeRotation(NewRotation);
}

void AGO_Door::DoorOpened()
{
	PrimaryActorTick.SetTickFunctionEnable(false);
	UE_LOG(LogTemp, Log, TEXT("Door Opened"));
}

void AGO_Door::CloseDoor()
{
	PrimaryActorTick.SetTickFunctionEnable(true);
	DoorTimeline.Reverse();

	FGameObjectState NewState;
	NewState.bIsActive = false;
	SetGameObjectState(NewState);
	
	UE_LOG(LogTemp, Log, TEXT("Door Closing!"));

	OnDoorClosed.Broadcast();
}

void AGO_Door::ActivateGameObject(const FGameObjectState& NewState)
{
	Super::ActivateGameObject(NewState);
	PrimaryActorTick.SetTickFunctionEnable(true);
	DoorTimeline.PlayFromStart();

	if (bResetDoorAfterActivation)
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&AGO_Door::CloseDoor,
			5.0f,
			false);
	}
}

// Called every frame
void AGO_Door::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (DoorTimeline.IsPlaying() || DoorTimeline.IsReversing())
	{
		DoorTimeline.TickTimeline(DeltaTime);
	}
}

