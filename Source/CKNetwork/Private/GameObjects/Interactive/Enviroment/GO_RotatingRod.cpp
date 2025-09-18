#include "CKNetwork/Pubilc/GameObjects/Interactive/Enviroment/GO_RotatingRod.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AGO_RotatingRod::AGO_RotatingRod()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root); 

	PivotRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PivotRoot"));
	PivotRoot->SetupAttachment(Root);

	RodRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RodRoot"));
	RodRoot->SetupAttachment(PivotRoot);

	Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
	Pillar->SetupAttachment(PivotRoot);

	RotatingRod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RotatingRod"));
	RotatingRod->SetupAttachment(RodRoot);

	// Optionally set default meshes if available
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Pillar->SetStaticMesh(CylinderMesh.Object);
		RotatingRod->SetStaticMesh(CylinderMesh.Object);
	}
}

void AGO_RotatingRod::BeginPlay()
{
	Super::BeginPlay();
	CurrentRotation = 0.0f;
}

void AGO_RotatingRod::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bReverseDirection) {
		CurrentRotation -= RotationSpeedDegreesPerSecond * DeltaTime;
	}
	else {
		CurrentRotation += RotationSpeedDegreesPerSecond * DeltaTime;
	}

	if (CurrentRotation >= 360.0f)
	{
		CurrentRotation -= 360.0f;
	}

	FQuat RotationQuat = FQuat(RotationAxis.GetSafeNormal(), FMath::DegreesToRadians(CurrentRotation));
	PivotRoot->SetRelativeRotation(RotationQuat);
}

void AGO_RotatingRod::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateRodTransform();
}

void AGO_RotatingRod::UpdateRodTransform()
{
	float RodScale = RodLength / CylinderHeight;
	RotatingRod->SetRelativeScale3D(FVector(0.25f, 0.25f, RodScale));

	// Orient rod to lie flat (cylinder by default stands vertical)
	RodRoot->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f)); // X aligned horizontally

	// Offset from pillar center depending on pivot mode
	FVector UnitAxis = RotationAxis.GetSafeNormal();

	// Perpendicular vector for offsetting the rod (in rotation plane)
	FVector PerpAxis = FVector::UpVector.Cross(UnitAxis).GetSafeNormal();

	// Handle edge case if axis is vertical (Z), use right vector
	if (PerpAxis.IsNearlyZero())
	{
		PerpAxis = FVector::RightVector;
	}

	// Calculate rod origin offset based on PivotRatio
	const float RodOffset = RodLength * (PivotRatio - 0.5f);
	const FVector Offset = -PerpAxis * RodOffset;

	// Apply height and set location
	FVector FinalLocation = Offset;
	FinalLocation.Z = RodHeight / 2;
	RodRoot->SetRelativeLocation(FinalLocation);

	// Default visual scale
	Pillar->SetRelativeScale3D(FVector(0.5f, 0.5f, RodHeight / CylinderHeight));
}