#pragma once
#include "CoreMinimal.h"
#include "FBaseEventState.generated.h"


USTRUCT(BlueprintType, Blueprintable)
struct FBaseEventState
{
	GENERATED_BODY()
	
	// Common properties all events might have
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Location;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 Timestamp;
	
	// Virtual destructor for proper cleanup
	virtual ~FBaseEventState() {}

};

USTRUCT(BlueprintType, Blueprintable)
struct FBallState : public FBaseEventState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Version = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InitialSpeed;
};