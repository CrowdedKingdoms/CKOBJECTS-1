// Fill out your copyright notice in the Description page of Project Settings.


#include "GameObjects/Interactive/Enviroment/GO_Light.h"

#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"


// Sets default values
AGO_Light::AGO_Light()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootSceneComponent);
	Light->Intensity = 0.0f;
	LightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LightMesh"));
	LightMesh->SetupAttachment(RootSceneComponent);
}

// Called when the game starts or when spawned
void AGO_Light::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGO_Light::ActivateGameObject(const FGameObjectState& NewState)
{
	Super::ActivateGameObject(NewState);

	if (!NewState.bIsActive)
	{
		// Turn On Light with Red Color
		TurnOnLight(FColor::Red, 2.5f);
		UE_LOG(LogTemp, Log, TEXT("Light turned on with RED"));
		// Turn it off after 2 seconds
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&AGO_Light::TurnOffLight,
			2.0f,
			false);
	}
	else
	{
		TurnOnLight(FColor::Green, 2.5f);
	}
	
}

void AGO_Light::TurnOnLight(const FColor NewLightColor, const float Intensity)
{
	Light->SetLightColor(NewLightColor);
	Light->Intensity = Intensity;
}

void AGO_Light::TurnOffLight()
{
	UE_LOG(LogTemp, Log, TEXT("Light turned off"));
	Light->Intensity = 0.0f;
	Light->SetLightColor(FColor::White);
}

// Called every frame
void AGO_Light::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

int32 AGO_Light::GetLightOrder()
{
	return LightOrder;
}

