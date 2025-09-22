#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkelotAnimCollection.h"
#include "SkelotWorld.h"
#include "SkelotUtils.h"
#include "Shared/Types/Structures/Actors/FActorState.h"
#include "Templates/SharedPointer.h"
#include "SkelotInstanceManager.generated.h"

class UActorServiceSubsystem;
class UVoxelWorldSubsystem;

// Global Vars for easer reusability  
namespace InstanceConstants
{
	constexpr float STANDING_HEIGHT_OFFSET = 89.0f;
	constexpr float CROUCHING_HEIGHT_OFFSET = 45.0f;
	constexpr float ROTATION_YAW_OFFSET = 90.0f;
	constexpr float LOCAL_UPDATE_INTERVAL = 0.2f;
	constexpr float TRANSFORM_INTERP_SPEED = 10.0f;
	constexpr float LOCAL_MOVEMENT_SPEED = 0.5f;
	constexpr float POSITION_TOLERANCE = 20.0f;
	constexpr float LOCAL_WANDERING_RANGE = 300.0f;
}

UENUM()
enum ERemoteAnimState : uint8 {

	// NONE
	None,
	
	// Base state
	Idle,

	// Crouching states
	CrouchingIdle,
	CrouchingForward,
	CrouchingLeft,
	CrouchingRight,
	CrouchingForwardLeft,
	CrouchingForwardRight,
	CrouchingBackward,
	CrouchingBackwardLeft,
	CrouchingBackwardRight,
	
	// Walking states
	Walking,
	WalkingLeft,
	WalkingRight,
	WalkingForwardLeft,
	WalkingForwardRight,
	WalkingBackward,
	WalkingBackwardLeft,
	WalkingBackwardRight,
    
	// Running states
	Running,
	RunningLeft,
	RunningRight,
	RunningForwardLeft,
	RunningForwardRight,
	RunningBackward,
	RunningBackwardLeft,
	RunningBackwardRight,
	
};

UENUM()
enum class EMovementDirection : uint8
{
	Forward,
	ForwardRight,
	Right,
	BackwardRight,
	Backward,
	BackwardLeft,
	Left,
	ForwardLeft
};

// For Replicated Instances
USTRUCT()
struct FRemoteActorState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector LastKnownLocation;

	UPROPERTY()
	FRotator LastKnownRotation;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	float LastUpdateTime;

	UPROPERTY()
	float TimeSinceLastUpdate = 0.0f;
	
	ERemoteAnimState CurrentAnimState = ERemoteAnimState::None;
};

// Data Wrapper for Replicated Instances
USTRUCT()
struct FRemoteInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	FSkelotInstanceHandle InstanceHandle;

	UPROPERTY()
	FRemoteActorState RemoteActorState;

	UPROPERTY()
	FString UUID;
	
};

// For Locally Created Instances
USTRUCT()
struct FLocalActorState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FRotator Rotation;

	// Movement Behavior
	UPROPERTY()
	float TimeSinceLastDirectionChange = 0.0f;

	UPROPERTY()
	float DirectionChangeInterval = 2.0f;

	UPROPERTY()
	float MovementSpeed = 300.0f;
};

// Data wrapper for Local Instances
USTRUCT()
struct FLocalInstanceData
{
	GENERATED_BODY()

	UPROPERTY()
	FSkelotInstanceHandle InstanceHandle;
	
	UPROPERTY()
	FLocalActorState LocalActorState;

	UPROPERTY()
	FString UUID;
};



UCLASS(Blueprintable, BlueprintType)
class  ASkelotInstanceManager : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	ASkelotInstanceManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//-- Add New Instance --
	UFUNCTION()
	void AddInstance(const FString& UUID, const FActorState& State);

	//-- Remove Instance --
	UFUNCTION()
	void RemoveInstance(const FString& UUID);

	//-- Update Instance --
	UFUNCTION()
	void UpdateInstance(const FString& UUID, const FActorState& State);


	//-- Local Instances Functions --
	
	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager")
	void AddLocalInstance(FTransform SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager")
	void RemoveLocalInstance();

	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager")
	void RemoveAllLocalInstances();
	
	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager|Actor Updates")
	void StartActorUpdates();
	
	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager|Actor Updates")
	void StopActorUpdates();

	UFUNCTION()
	bool IsLocalInstance(const FString& UUID) const;
	
	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager")
	void SetVoxelControllerReference(UVoxelWorldSubsystem* InVoxelController) {VoxelWorldControllerReference = InVoxelController;}

	UFUNCTION(BlueprintCallable, Category = "Skelot Instance Manager")
	void SetActorServiceReference(UActorServiceSubsystem* InActorService){ActorServiceReference = InActorService;}
	
	// Properties
	UPROPERTY(EditAnywhere)
	TArray<USkelotRenderParams*> RenderParams;
	
	UPROPERTY(BlueprintReadOnly, Category = "Skelot Instance Manager")
	int32 LocalInstanceCounter = 0;

	UPROPERTY(EditAnywhere, Category = "Skelot Instance Manager")
	TMap<TEnumAsByte<ERemoteAnimState>, UAnimSequenceBase*> AnimationMap;
	
protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	static EMovementDirection GetMovementDirection(const FVector& FacingDirection, const FVector& MovementDirection);

	static ERemoteAnimState SelectAnimationState(const float Speed, bool bIsCrouching, const EMovementDirection Direction);

	void SendActorUpdates() const;
	
	// Skelot World Reference -- used for managing instances
	UPROPERTY()
	ASkelotWorld* SkelotWorld;

	// Map to store replicated instances of characters (Uses SharedPtr for Shared Ownership with Array)
	TMap<FString, TSharedPtr<FRemoteInstanceData>> RemoteInstanceMap;

	// Data Wrapper for storing replicated instances
	TArray<TSharedPtr<FRemoteInstanceData>> RemoteInstancesData;

	// Data Wrapper for storing local instances
	UPROPERTY()
	TArray<FLocalInstanceData> LocalInstancesData;

	UPROPERTY()
	TArray<FString> LocalInstancesUUIDTracking;

	UPROPERTY()
	UVoxelWorldSubsystem* VoxelWorldControllerReference;

	UPROPERTY()
	UActorServiceSubsystem* ActorServiceReference;

	UPROPERTY()
	FTimerHandle ActorUpdateTimerHandle;
};
