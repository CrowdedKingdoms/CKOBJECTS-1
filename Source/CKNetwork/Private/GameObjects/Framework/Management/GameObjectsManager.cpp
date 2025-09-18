// Fill out your copyright notice in the Description page of Project Settings.


#include "CKNetwork/Pubilc/GameObjects/Framework/Management/GameObjectsManager.h"
#include "CKNetwork/Pubilc/GameObjects/Framework/Base/ActivatorBase.h"
#include "CKNetwork/Pubilc/GameObjects/Interactive/Physics/Ball.h"
#include "Kismet/GameplayStatics.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/GameObjectsServiceSubsystem.h"


// Sets default values
AGameObjectsManager::AGameObjectsManager(): GameObjectService(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AGameObjectsManager::InitializeEventMaps()
{
	// Add UUID Prefixes
	EventTypeMap.Add("BALL", EEventType::Ball);

	// Add Event Classes
	EventActorClassMap.Add(EEventType::Ball, ABall::StaticClass());
	
}

void AGameObjectsManager::SetGameObjectsServiceReference(UGameObjectsServiceSubsystem* InGOService)
{
	GameObjectService = InGOService;
}

void AGameObjectsManager::SpawnBall(FBallState BallState)
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("GameObjectsManager: GetWorld() is null."));
	}

	FTransform SpawnTransform = FTransform(BallState.Rotation, BallState.Location, BallState.Scale);
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	ABall* SpawnedBall = GetWorld()->SpawnActor<ABall>(ABall::StaticClass(), SpawnTransform, SpawnParams);
	if (SpawnedBall)
	{
		SpawnedBall->SetSpawnVariables(BallState.Velocity, BallState.InitialSpeed, 0, 0);
		SpawnedBall->FinishSpawning(SpawnTransform);
		//UE_LOG(LogTemp, Log, TEXT("GameObjectsManager: Spawned Ball. Location: %f, %f, %f"), SpawnedBall->GetActorLocation().X, SpawnedBall->GetActorLocation().Y, SpawnedBall->GetActorLocation().Z);
	}
}

void AGameObjectsManager::SpawnBall(const FBallState& BallState) const
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("GameObjectsManager: GetWorld() is null."));
		return;
	}

	// Get time delta in milliseconds
	const int64 CurrentTimeInMilliseconds = FDateTime::UtcNow().ToUnixTimestamp() * 1000 + FDateTime::UtcNow().GetMillisecond();
	const int64 TimeDelta = CurrentTimeInMilliseconds - BallState.Timestamp;

	// Convert ms to seconds and clamp
	float TimeDeltaSeconds = TimeDelta / 1000.0f;
	TimeDeltaSeconds = FMath::Clamp(TimeDeltaSeconds, 0.0f, 1.0f);

	// World-space velocity
	const FVector WorldVelocity = BallState.Rotation.RotateVector(BallState.Velocity);

	// Estimate the position the ball should be at
	const FVector EstimatedLocation = BallState.Location + WorldVelocity * TimeDeltaSeconds;

	// Spawn transform
	const FTransform SpawnTransform(BallState.Rotation, EstimatedLocation, BallState.Scale);

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bDeferConstruction = true;

	// Spawn the ball
	ABall* SpawnedBall = GetWorld()->SpawnActor<ABall>(ABall::StaticClass(), SpawnTransform, SpawnParams);

	if (SpawnedBall)
	{
		// Apply initial velocity (assuming your ball class has this method)
		SpawnedBall->SetSpawnVariables(BallState.Velocity*BallState.InitialSpeed, BallState.InitialSpeed, 0, 0);

		SpawnedBall->FinishSpawning(SpawnTransform);
	}
}



// Called when the game starts or when spawned
void AGameObjectsManager::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActivatorBase::StaticClass(), Actors);

	// Convert the array to AActivatorBase pointers if needed
	TArray<AActivatorBase*> ActivatorActors;
	for (AActor* Actor : Actors)
	{
		if (AActivatorBase* Activator = Cast<AActivatorBase>(Actor))
		{
			ActivatorActors.Add(Activator);
		}
	}

	// Add the activators to the map
	for (AActivatorBase* Activator : ActivatorActors)
	{
		FString NewUUId = FGuid::NewGuid().ToString();
		ActivatorsMap.Add(NewUUId, Activator);
		Activator->SetGameObjectManagerReference(this);
		Activator->SetActivatorUUID(NewUUId);
	}

	// Ensuring Game Service Object exists and reference is set
	if (IsValid(GameObjectService))
	{
		// Passing Reference to the GameObjectService
		GameObjectService->SetGameObjectsManager(this);
		UE_LOG(LogTemp, Log, TEXT("GameObjectsManager: Service_GameObjectService is valid and reference is set."));
	}

	OnGameObjectActivationNotificationReceived.AddDynamic(this, &AGameObjectsManager::FindAndActivateObject);
	OnBallEventReceived.AddUObject(this, &AGameObjectsManager::ProcessBallEvent);

	InitializeEventMaps();
}

void AGameObjectsManager::FindAndActivateObject(const FString& ObjectUUID, const FGameObjectState& NewState)
{
	UE_LOG(LogTemp, Log, TEXT("GameObjectsManager: FindAndActivateObject Called"));
	if (ActivatorsMap.Contains(ObjectUUID))
	{
		UE_LOG(LogTemp, Log, TEXT("GameObjectsManager: ObjectUUID found in the map."));
		ActivatorsMap[ObjectUUID]->ActivateEvent_Implementation(NewState);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GameObjectsManager: ObjectUUID not found in the map."));
	}
}

void AGameObjectsManager::ProcessBallEvent(const FString& EventUUID, const FBallState& BallState) const
{
	if (DoesEventExist(EventUUID))
	{
		SpawnBall(BallState);
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("GameObjectsManager: EventUUID not found in the map."));
}


bool AGameObjectsManager::DoesEventExist(const FString& EventUUID) const
{
	UE_LOG(LogTemp, Log, TEXT("GameObjectsManager: FindAndActivateEvent Called for UUID: %s"), *EventUUID);
	
	bool TypeFound = false;

	for (const auto& Prefix: EventTypeMap)
	{
		if (EventUUID.StartsWith(Prefix.Key))
		{
			TypeFound = true;
			break;
		}
	}

	if (!TypeFound)
	{
		UE_LOG(LogTemp, Error, TEXT("GameObjectsManager: EventUUID not found in the map."));
		return false;
	}
	
	UE_LOG(LogTemp, Log, TEXT("GameObjectsManager: EventUUID found in the map."));
	return true;
	
}

// Called every frame
void AGameObjectsManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameObjectsManager::DispatchActivationRequest(const int64 MapId, const int64 ChunkX, const int64 ChunkY,
	const int64 ChunkZ, const FString& ActivatorUUID, const uint16 EventType, const FGameObjectState& State) const
{
	GameObjectService->SendGameObjectActivationRequest(MapId, ChunkX, ChunkY, ChunkZ, ActivatorUUID, EventType, State);
}

