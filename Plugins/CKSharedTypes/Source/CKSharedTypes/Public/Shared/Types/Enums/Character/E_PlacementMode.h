﻿#pragma once

#include "CoreMinimal.h"
#include "E_PlacementMode.generated.h"

UENUM(BlueprintType)
enum class E_PlacementMode : uint8
{
	None		UMETA(DisplayName = "No placement"),
	Voxel		UMETA(DisplayName = "Placing Voxel"),
	VLO			UMETA(DisplayName = "Placing Voxel Like Object"),
	StaticMesh	UMETA(DisplayName = "Placing Static Mesh"),
	ActorBP		UMETA(DisplayName = "Placing Actor Blueprint")
};