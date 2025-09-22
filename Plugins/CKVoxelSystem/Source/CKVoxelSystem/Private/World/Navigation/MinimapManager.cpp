// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Navigation//MinimapManager.h"

#include "Blueprint/UserWidget.h"
#include "Shared/Types/Interfaces/UI/Minimap/MinimapWidgetInterface.h"


// Sets default values
AMinimapManager::AMinimapManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

FVector2D AMinimapManager::GetPlayerMinimapPosition2D(const FString PlayerUUID, const FVector OwningPlayerLocation) const
{
	const float PixelRadius = MinimapSize.X * 0.5f;
	const FVector2D MinimapCenter = MinimapSize * 0.5f;
    
	const FVector ActorLocation = PlayerLocations[PlayerUUID];
    
	// Swap X and Y coordinates to match world orientation to minimap orientation
	// Using Y as forward (vertical on minimap) and X as right (horizontal on minimap)
	const FVector2D Relative = FVector2D(ActorLocation.Y - OwningPlayerLocation.Y, -(ActorLocation.X - OwningPlayerLocation.X));
    
	// Or if that doesn't work, try negating one axis:
	// const FVector2D Relative = FVector2D(-(ActorLocation.Y - OwningPlayerLocation.Y), ActorLocation.X - OwningPlayerLocation.X);
    
	const FVector2D ScaledPosition = Relative / WorldRadius;
	const FVector2D MinimapPosition = MinimapCenter + (ScaledPosition * PixelRadius);
    
	if ((MinimapPosition - MinimapCenter).SizeSquared() > FMath::Square(PixelRadius))
	{
		///FVector2D Direction = (MinimapPosition - MinimapCenter).GetSafeNormal();
		// hide the player altogether
		return FVector2D(-10000, -10000);
	}

	return MinimapPosition;
}

void AMinimapManager::AddPlayerToMinimap(const FString PlayerUUID, const FVector ActorLocation)
{
	if (!PlayerLocations.Contains(PlayerUUID))
	{
		PlayerLocations.Add(PlayerUUID, ActorLocation);
		IMinimapWidgetInterface::Execute_AddPlayerToMinimapWidget(MinimapWidget, PlayerUUID);
	}
}

void AMinimapManager::UpdatePlayerLocation(const FString PlayerUUID, const FVector ActorLocation)
{
	if (PlayerLocations.Contains(PlayerUUID))
	{
		PlayerLocations[PlayerUUID] = ActorLocation;
		IMinimapWidgetInterface::Execute_UpdatePlayerLocationOnMinimapWidget(MinimapWidget);
	}
}

void AMinimapManager::RemovePlayerFromMinimap(const FString PlayerUUID)
{
	if (PlayerLocations.Contains(PlayerUUID))
	{
		PlayerLocations.Remove(PlayerUUID);
		IMinimapWidgetInterface::Execute_RemovePlayerFromMinimapWidget(MinimapWidget, PlayerUUID);
	}
}

// Called when the game starts or when spawned
void AMinimapManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMinimapManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

