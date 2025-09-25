// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibraries/Generic/FL_Generic.h"
#include <cmath>

FString UFL_Generic::GenerateUUID()
{
	const FGuid NewGuid = FGuid::NewGuid();
	return NewGuid.ToString(EGuidFormats::DigitsWithHyphens);
}



void UFL_Generic::CalculateChunkCoordinatesAtWorldLocation(const FVector& WorldLocation, int64& ChunkX, int64& ChunkY, int64& ChunkZ)
{
	constexpr double ChunkSize = 1600.0;
	
	double X = static_cast<double>(WorldLocation.X) / ChunkSize;
	double Y = static_cast<double>(WorldLocation.Y) / ChunkSize;
	double Z = static_cast<double>(WorldLocation.Z) / ChunkSize;

	ChunkX = static_cast<int64>(std::floor(X));
	ChunkY = static_cast<int64>(std::floor(Y));
	ChunkZ = static_cast<int64>(std::floor(Z));
}

void UFL_Generic::CalculateVoxelCoordinatesAtWorldLocation(const FVector& WorldLocation, int32& OutX, int32& OutY, int32& OutZ)
{
	FVector Lcl_WorldLocation = WorldLocation;
	int32 X = 0;
	int32 Y = 0;
	int32 Z = 0;
	
	constexpr double ChunkSize = 1600.0;
	
	X= std::fmod((Lcl_WorldLocation.X), ChunkSize);
	Y = std::fmod((Lcl_WorldLocation.Y), ChunkSize);
	Z= std::fmod((Lcl_WorldLocation.Z), ChunkSize);

	if (X < 0.0) X += ChunkSize;
	if (Y < 0.0) Y += ChunkSize;
	if (Z < 0.0) Z += ChunkSize;

	OutX = X / 100;
	OutY = Y / 100;
	OutZ = Z / 100;
}


bool UFL_Generic::DeprojectScreenCenterToWorldPoint(APlayerController* PlayerController, float Distance, FVector& OutWorldPoint)
{
	OutWorldPoint = FVector::ZeroVector;

	if (!IsValid(PlayerController))
	{
		UE_LOG(LogTemp, Warning, TEXT("DeprojectScreenCenterToWorldPoint: PlayerController is invalid"));
		return false;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	
	PlayerController->GetViewportSize(ViewportX, ViewportY);

	if (ViewportX <= 0 || ViewportY <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeprojectScreenCenterToWorldPoint: Invalid viewport size (%d, %d)"), ViewportX, ViewportY);
		return false;
	}

	const float ScreenX = static_cast<float>(ViewportX) * 0.5f;
	const float ScreenY = static_cast<float>(ViewportY) * 0.5f;

	FVector WorldOrigin;
	FVector WorldDirection;
	// Deproject screen center to a world origin + direction (world space)
	const bool bSuccess = PlayerController->DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldOrigin, WorldDirection);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("DeprojectScreenCenterToWorldPoint: DeprojectScreenPositionToWorld failed"));
		return false;
	}

	OutWorldPoint = WorldOrigin + WorldDirection * Distance;
	return true;
}