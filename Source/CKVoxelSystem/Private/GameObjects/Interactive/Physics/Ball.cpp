// Fill out your copyright notice in the Description page of Project Settings.


#include "GameObjects/Interactive/Physics/Ball.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Shared/Types/Interfaces/Interaction/Activatable.h"
#include "Shared/Types/Interfaces/Player/HitResponseInterface.h"
#include "Shared/Types/Interfaces/Player/PlayerInterface.h"

// Sets default values
ABall::ABall()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetCollisionProfileName("Custom");
	CollisionComponent->SetSphereRadius(16.0f);

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Collision Enabled: Query Only
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic); // Object Type: WorldDynamic

	// Set individual collision responses
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);  // Visibility: Block
	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);     // Camera: Block

	// Block these channels
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // If "Projectile" is a custom channel, it would be a trace channel
	
	// Block Pawn
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	RootComponent = CollisionComponent;
	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SphereMesh"));
	SphereMesh->SetupAttachment(CollisionComponent);
	SphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/FPWeapon/Mesh/FirstPersonProjectileMesh.FirstPersonProjectileMesh")))
	{
		SphereMesh->SetStaticMesh(Mesh);
	}

	SphereMesh->SetRelativeScale3D(FVector(0.1, 0.1, 0.1));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->StopSimulating(FHitResult());
}

FVector ABall::GetActorOriginalLocation_Implementation()
{
	return GetActorLocation();
}

// Called when the game starts or when spawned
void ABall::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsAccelerating && ProjectileMovement)
	{
		AccelerationTimeRemaining -= DeltaTime;

		if (AccelerationTimeRemaining <= 0.0f)
		{
			bIsAccelerating = false;

			ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * TargetSpeed;
			ProjectileMovement->UpdateComponentVelocity();
		}
		else
		{
			// Calculate current progress through acceleration
			float CurrentProgress = (TotalAccelerationDuration - AccelerationTimeRemaining) / TotalAccelerationDuration;
            
			// Calculate current speed (lerp from initial to target)
			float CurrentSpeed = FMath::Lerp(
				ProjectileMovement->InitialSpeed,
				TargetSpeed,
				CurrentProgress
			);
            
			// Update velocity
			ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * CurrentSpeed;
			ProjectileMovement->UpdateComponentVelocity();
		}
	}
}


void ABall::SetSpawnVariables(const FVector NewVelocity, const float NewSpeed, float RemainingAccelTime, float TotalAccelTime)
{
	ProjectileMovement->Velocity = NewVelocity;
	ProjectileMovement->InitialSpeed = NewSpeed;

	bIsAccelerating = (RemainingAccelTime > 0.0f);
	TargetSpeed = NewSpeed;
	AccelerationTimeRemaining = RemainingAccelTime;
	TotalAccelerationDuration = TotalAccelTime;

	if (FMath::IsNearlyEqual(ProjectileMovement->InitialSpeed, NewSpeed))
	{
		bIsAccelerating = false;
	}
	
	ProjectileMovement->UpdateComponentVelocity();
}

void ABall::NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	// Get the world to access the timer manager

	// For handling hits to player
	if (Other->GetClass()->ImplementsInterface(UPlayerInterface::StaticClass()))
	{
		AActor* GameZone = IPlayerInterface::Execute_GetPlayerMinigameZone(Other);
		if (IsValid(GameZone))
		{
			IHitResponseInterface::Execute_HandlePlayerHit(GameZone, Other, GetVelocity());
		}
	}

	// For handling hits for activatable object types
	if (Other->GetClass()->ImplementsInterface(UActivatable::StaticClass()))
	{
		UE_LOG(LogTemp, Log, TEXT("Ball: Hit activatable object"));
		AActor* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		AActor* GameZone = IPlayerInterface::Execute_GetPlayerMinigameZone(Player);
		if (IsValid(GameZone))
		{
			UE_LOG(LogTemp, Log, TEXT("Ball: Hit activatable object in game zone"));
			IHitResponseInterface::Execute_HandlePlayerHit(GameZone, Other, FVector::ZeroVector);
		}
	}
	
	if (!bFirstContact)
	{
		bFirstContact = true;
		ProjectileMovement->ProjectileGravityScale = 1.0f;
		if (const UWorld* World = GetWorld())
		{
			// Set a timer with lambda to destroy the ball after 10 seconds
			FTimerHandle DestroyTimerHandle;
			World->GetTimerManager().SetTimer(
				DestroyTimerHandle,
				[this]()
				{
					Destroy();
				},
				10.0f,
				false
			);
		}
	}
}


