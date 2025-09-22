// Fill out your copyright notice in the Description page of Project Settings.

#include "GameObjects/Interactive/Gameplay/GO_Checkpoint.h"
#include "GameFramework/Character.h"

// Sets default values
AGO_Checkpoint::AGO_Checkpoint()
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

	LeftPillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPillarMesh"));
	LeftPillar->SetupAttachment(RootSceneComponent);

	RightPillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPillarMesh"));
	RightPillar->SetupAttachment(RootSceneComponent);

	Banner = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BannerMesh"));
	Banner->SetupAttachment(RootSceneComponent);
}

// Called when the game starts or when spawned
void AGO_Checkpoint::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGO_Checkpoint::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGO_Checkpoint::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (ACharacter* Player = Cast<ACharacter>(OtherActor))
	{
		APlayerController* PC = Cast<APlayerController>(Player->GetController());
		if (PC)
		{
			UE_LOG(LogTemp, Log, TEXT("Checkpoint Crossed"));
			OnCheckpointCrossed.Broadcast(bIsFinalCheckpoint);
		}
	}
}

bool AGO_Checkpoint::IsFinal() const
{
	return bIsFinalCheckpoint;
}

void AGO_Checkpoint::ActivateGameObject(const FGameObjectState& NewState)
{
	Super::ActivateGameObject(NewState);
	UE_LOG(LogTemp, Log, TEXT("Checkpoint Activated"));
	OnCheckpointActivated.Broadcast(this);
}

void AGO_Checkpoint::OnConstruction(const FTransform& Transform)
{
	if (PillarMesh)
	{
		LeftPillar->SetStaticMesh(PillarMesh);
		RightPillar->SetStaticMesh(PillarMesh);
	}

	if (BannerMesh)
	{
		Banner->SetStaticMesh(BannerMesh);
	}

	//FVector LeftLocation = FVector(0, -GateWidth * 0.5f, 0);
	//FVector RightLocation = FVector(0, GateWidth * 0.5f, 0);
	//FVector CrossbarLocation = FVector(0, 0, GateHeight);

	//LeftPillar->SetRelativeLocation(LeftLocation);
	//RightPillar->SetRelativeLocation(RightLocation);
	//Banner->SetRelativeLocation(CrossbarLocation);

	//FVector CrossbarScale(1, GateWidth / 100.0f, 1); // assuming default 100cm width
	//Banner->SetRelativeScale3D(CrossbarScale);
}