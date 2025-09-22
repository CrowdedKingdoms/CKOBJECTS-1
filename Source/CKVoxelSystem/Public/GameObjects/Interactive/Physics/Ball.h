// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Shared/Types/Interfaces/World/OriginRebasable.h"
#include "Ball.generated.h"

UCLASS(Blueprintable, BlueprintType)
class  ABall : public AActor, public IOriginRebasable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABall();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SetSpawnVariables(const FVector NewVelocity, const float NewSpeed, float RemainingAccelTime, float TotalAccelTime);

	UPROPERTY(BlueprintReadWrite, Category= "Ball", meta=(ExposeOnSpawn = true))
	FVector BaseVelocity = FVector(0, 0, 0);

	UPROPERTY(BlueprintReadWrite, Category= "Ball", meta=(ExposeOnSpawn = true))
	float BaseSpeed = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Ball")
	FVector GetActorOriginalLocation_Implementation() override;

protected:
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Hit event
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	USphereComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	UStaticMeshComponent* SphereMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	bool bIsAccelerating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	float TargetSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	float AccelerationTimeRemaining;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	float TotalAccelerationDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
	bool bFirstContact = false;
	
};

