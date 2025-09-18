#pragma once

#include "CoreMinimal.h"
#include "CKNetwork/Pubilc/GameObjects/Framework/Base/GameObjectBase.h"
#include "GO_RotatingRod.generated.h"

UCLASS()
class AGO_RotatingRod : public AGameObjectBase
{
	GENERATED_BODY()

public:
	AGO_RotatingRod();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod")
	USceneComponent* PivotRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod")
	USceneComponent* RodRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod")
	UStaticMeshComponent* Pillar;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod")
	UStaticMeshComponent* RotatingRod;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod")
	float RotationSpeedDegreesPerSecond = 90.0f;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod|Config")
	FVector RotationAxis = FVector(0, 0, 1); // Default Z-axis

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod|Config")
	float CylinderHeight = 100.0f; // Mesh height

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod|Config")
	float RodLength = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod|Config")
	float RodHeight = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod|Config")
	bool bReverseDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Object|Rotating Rod|Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PivotRatio = 0.5f;

private:
	float CurrentRotation;
	void UpdateRodTransform();
};
