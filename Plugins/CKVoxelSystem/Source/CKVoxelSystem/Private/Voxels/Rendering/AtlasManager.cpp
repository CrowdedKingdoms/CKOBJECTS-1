// Fill out your copyright notice in the Description page of Project Settings.
#include "Voxels/Rendering/AtlasManager.h"


// Sets default values
AAtlasManager::AAtlasManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

bool AAtlasManager::DoesAtlasExist(const int64 AtlasID) const
{
	if (AtlasMap.Contains(AtlasID))
	{
		return true;
	}

	return false;
}

UTexture* AAtlasManager::GetAtlas(const int64 AtlasID) const
{
	return AtlasMap[AtlasID];
}

// Called when the game starts or when spawned
void AAtlasManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAtlasManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

