

#include "Voxels/Core/GhostPlacement.h"

#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AGhostPlacement::AGhostPlacement()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	if (ProceduralMesh)
	{
		ProceduralMesh->SetupAttachment(DefaultSceneRoot);
	}

	GhostStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GhostStaticMesh"));
	if (GhostStaticMesh)
	{
		GhostStaticMesh->SetupAttachment(DefaultSceneRoot);
	}
}

void AGhostPlacement::BeginPlay()
{
	Super::BeginPlay();

	if (GhostMaterialReference && GhostStaticMesh)
	{
		GhostDynamicMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(GhostMaterialReference, this);
		if (GhostDynamicMaterialInstanceDynamic)
		{
			GhostStaticMesh->SetMaterial(0, GhostDynamicMaterialInstanceDynamic);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AGhostPlacement Activated: "));
}

void AGhostPlacement::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

