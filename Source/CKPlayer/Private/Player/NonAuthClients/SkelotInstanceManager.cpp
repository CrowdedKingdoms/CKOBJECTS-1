#include "Player/NonAuthClients/SkelotInstanceManager.h"
#include "CKVoxelSystem/Public/Voxels/Core/VoxelWorldSubsystem.h"
#include "CKNetwork/Pubilc/Network/Services/GameData/ActorServiceSubsystem.h"


// Sets default values
ASkelotInstanceManager::ASkelotInstanceManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SkelotWorld = nullptr;
}

// Called when the game starts or when spawned
void ASkelotInstanceManager::BeginPlay()
{
	Super::BeginPlay();

	SkelotWorld = ASkelotWorld::Get(GetWorld());
}

// Called every frame
void ASkelotInstanceManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	constexpr float SmoothingSpeed = 10.0f;

	// For Replicated Instances Only
	ParallelFor(RemoteInstancesData.Num(), [this, DeltaTime](const int32 Index)
	{
		FSkelotInstanceHandle& Handle = RemoteInstancesData[Index]->InstanceHandle;
		FRemoteActorState& State = RemoteInstancesData[Index]->RemoteActorState;

		State.TimeSinceLastUpdate += DeltaTime;

		// --- Location ---
		const FVector PredictedLocation = State.LastKnownLocation + (State.Velocity * State.TimeSinceLastUpdate);
		const FVector CurrentLocation = SkelotWorld->GetInstanceLocation(Handle.InstanceIndex);
		const FVector SmoothedLocation = FMath::Lerp(CurrentLocation, PredictedLocation, DeltaTime * SmoothingSpeed);

		// --- Rotation ---
		const FRotator CurrentRotation = FQuat(SkelotWorld->GetInstanceRotation(Handle.InstanceIndex)).Rotator();
		const FRotator TargetRotation = State.LastKnownRotation;
		const FRotator SmoothedRotation = FMath::Lerp(CurrentRotation, TargetRotation, DeltaTime * SmoothingSpeed);

		SkelotWorld->SetInstanceLocationAndRotation(
			Handle.InstanceIndex,
			SmoothedLocation,
			FQuat4f(FQuat(SmoothedRotation))
		);
	});


	// For Locally Spawned Instances Only
	ParallelFor(LocalInstanceCounter, [this, DeltaTime](const int32 Index)
	{
		FLocalInstanceData& LocalInstanceData = LocalInstancesData[Index];

		FLocalActorState& LocalActorState = LocalInstanceData.LocalActorState;

		LocalActorState.TimeSinceLastDirectionChange += DeltaTime;

		if (LocalActorState.TimeSinceLastDirectionChange >= LocalActorState.DirectionChangeInterval)
		{
			LocalActorState.TimeSinceLastDirectionChange = 0.0f;
			LocalActorState.DirectionChangeInterval = FMath::RandRange(1.5f, 3.0f);

			const float Angle = FMath::RandRange(0.0f, 360.0f);
			LocalActorState.Velocity = FRotator(0.0f, Angle, 0.0f).Vector() * LocalActorState.MovementSpeed;
		}

		LocalActorState.Location += LocalActorState.Velocity * DeltaTime;

		if (!LocalActorState.Velocity.IsNearlyZero())
		{
			FVector Forward2D = LocalActorState.Velocity;
			Forward2D.Z = 0.0f;

			if (!Forward2D.IsNearlyZero())
			{
				const FRotator TargetRotation = Forward2D.Rotation() - FRotator(
					0.0f, InstanceConstants::ROTATION_YAW_OFFSET, 0.0f);
				LocalActorState.Rotation = FMath::RInterpTo(LocalActorState.Rotation, TargetRotation, DeltaTime, 10.0f);
			}
		}

		SkelotWorld->SetInstanceLocationAndRotation(
			LocalInstanceData.InstanceHandle.InstanceIndex,
			LocalActorState.Location,
			FQuat4f(FQuat(LocalActorState.Rotation))
		);
	});
}

void ASkelotInstanceManager::AddInstance(const FString& UUID, const FActorState& State)
{
	if (RemoteInstanceMap.Contains(UUID))
	{
		UE_LOG(LogTemp, Log, TEXT("Instance already exists: %s"), *UUID);
		return;
	}

	if (!SkelotWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("SkelotWorld is not valid"));
		return;
	}

	// Set the Spawn Transform
	FTransform InstanceTransform;

	InstanceTransform.SetLocation(State.Position - FVector(0.0f, 0.0f, InstanceConstants::STANDING_HEIGHT_OFFSET));
	InstanceTransform.SetRotation(
		(State.Rotation - FRotator(0.0f, InstanceConstants::ROTATION_YAW_OFFSET, 0.0f)).Quaternion());


	// Create the Instance
	const FSkelotInstanceHandle NewInstanceHandle = SkelotWorld->CreateInstance(
		InstanceTransform, FSkelotUtils::GetArrayElementRandom(RenderParams));
	
	// Initialize State
	const TSharedPtr<FRemoteInstanceData> RemoteInstanceData = MakeShared<FRemoteInstanceData>();
	RemoteInstanceData->UUID = UUID;
	RemoteInstanceData->InstanceHandle = NewInstanceHandle;

	// Set up State
	FRemoteActorState RemoteActorState;
	RemoteActorState.LastKnownLocation = InstanceTransform.GetLocation();
	RemoteActorState.LastKnownRotation = InstanceTransform.GetRotation().Rotator();
	RemoteActorState.Velocity = FVector::ZeroVector;
	RemoteActorState.LastUpdateTime = FPlatformTime().Seconds();
	RemoteActorState.TimeSinceLastUpdate = 0.0f;

	// Set the reference for state instance
	RemoteInstanceData->RemoteActorState = RemoteActorState;

	// Add to map and array for tracking 
	RemoteInstanceMap.Add(UUID, RemoteInstanceData);
	RemoteInstancesData.Add(RemoteInstanceData);

	UE_LOG(LogTemp, Log, TEXT("Added Instance: %s"), *UUID);

	// Play Random Animation
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, NewInstanceHandle]()
	{
		SkelotWorld->InstancePlayAnimation(
			NewInstanceHandle.InstanceIndex, FSkelotAnimPlayParams{
				.Animation = AnimationMap[Idle],
				.bLoop = true
			});
	}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}

void ASkelotInstanceManager::RemoveInstance(const FString& UUID)
{
	if (!RemoteInstanceMap.Contains(UUID))
	{
		UE_LOG(LogTemp, Log, TEXT("Instance does not exist: %s"), *UUID);
		return;
	}

	if (!SkelotWorld)
	{
		UE_LOG(LogTemp, Log, TEXT("Skelot World is invalid. Returning..."));
		return;
	}

	// Get the instance
	const TSharedPtr<FRemoteInstanceData> RemoteInstanceData = RemoteInstanceMap[UUID];
	RemoteInstancesData.Remove(RemoteInstanceData);

	// Destroy Instance
	SkelotWorld->DestroyInstance(RemoteInstanceData->InstanceHandle);
	RemoteInstanceMap.Remove(UUID);
	UE_LOG(LogTemp, Log, TEXT("Removed Instance: %s"), *UUID);
}

void ASkelotInstanceManager::UpdateInstance(const FString& UUID, const FActorState& State)
{
	if (!RemoteInstanceMap.Contains(UUID))
	{
		UE_LOG(LogTemp, Log, TEXT("Instance does not exist: %s"), *UUID);
		return;
	}

	if (!SkelotWorld)
	{
		UE_LOG(LogTemp, Log, TEXT("Skelot World is invalid. Returning..."));
		return;
	}

	// Updating and storing the state variables
	const TSharedPtr<FRemoteInstanceData> InstanceData = RemoteInstanceMap[UUID];
	FRemoteActorState& StoredState = InstanceData->RemoteActorState;

	StoredState.LastKnownLocation = State.bCrouch
		                                ? State.Position - FVector(0, 0, InstanceConstants::CROUCHING_HEIGHT_OFFSET)
		                                : State.Position - FVector(
			                                0.0f, 0.0f, InstanceConstants::STANDING_HEIGHT_OFFSET);

	StoredState.LastKnownRotation = State.Rotation - FRotator(0.0f, InstanceConstants::ROTATION_YAW_OFFSET, 0.0f);
	StoredState.Velocity = State.Velocity;
	StoredState.LastUpdateTime = FPlatformTime().Seconds();
	StoredState.TimeSinceLastUpdate = 0.0f;

	// Get the 2D velocity (ignore vertical component)
	const FVector Velocity2D = FVector(State.Velocity.X, State.Velocity.Y, 0.0f);
	const float Speed = Velocity2D.Size();

	const bool bIsCrouching = State.bCrouch;

	ERemoteAnimState DesiredState = Idle;

	if (Speed >= 150.0f || bIsCrouching)
	{
		const FVector MovementDirection = Velocity2D.GetSafeNormal();
		const FVector FacingDirection = State.Rotation.Vector().GetSafeNormal2D();
		const EMovementDirection Direction = GetMovementDirection(FacingDirection, MovementDirection);
		DesiredState = SelectAnimationState(Speed, bIsCrouching, Direction);
	}

	// Avoid playing the same animation repeatedly
	if (InstanceData->RemoteActorState.CurrentAnimState != DesiredState)
	{
		InstanceData->RemoteActorState.CurrentAnimState = DesiredState;

		FSkelotAnimPlayParams Params;
		Params.bLoop = true;
		Params.Animation = AnimationMap[DesiredState];

		// Play the selected animation
		UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, InstanceData, Params]()
		{
			SkelotWorld->InstancePlayAnimation(InstanceData->InstanceHandle.InstanceIndex, Params);
		}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
	}
}

void ASkelotInstanceManager::AddLocalInstance(const FTransform SpawnTransform)
{
	if (!SkelotWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("SkelotWorld is not valid. Returning early..."));
		return;
	}

	FLocalActorState LocalActorState;

	LocalActorState.Location = SpawnTransform.GetLocation();
	LocalActorState.MovementSpeed = FMath::RandRange(300.0f, 500.0f);
	LocalActorState.DirectionChangeInterval = FMath::RandRange(1.5f, 3.0f);

	const float Angle = FMath::RandRange(0.0f, 360.0f);
	LocalActorState.Velocity = FRotator(0.0f, Angle, 0.0f).Vector() * LocalActorState.MovementSpeed;
	LocalActorState.Rotation = LocalActorState.Velocity.Rotation() - FRotator(
		0.0f, InstanceConstants::ROTATION_YAW_OFFSET, 0.0f);

	const FSkelotInstanceHandle NewInstanceHandle = SkelotWorld->CreateInstance(
		SpawnTransform, FSkelotUtils::GetArrayElementRandom(RenderParams));

	SkelotWorld->InstancePlayAnimation(
		NewInstanceHandle.InstanceIndex, FSkelotAnimPlayParams{
			.Animation = AnimationMap[Running],
			.bLoop = true
		});


	const FString UUID = FGuid::NewGuid().ToString();

	FLocalInstanceData LocalInstanceData;

	LocalInstanceData.InstanceHandle = NewInstanceHandle;
	LocalInstanceData.UUID = UUID;
	LocalInstancesUUIDTracking.Add(UUID);
	LocalInstanceData.LocalActorState = LocalActorState;

	LocalInstancesData.Add(LocalInstanceData);
	LocalInstanceCounter++;

	UE_LOG(LogTemp, Log, TEXT("Added Local Instance: %s"), *UUID);
}

void ASkelotInstanceManager::RemoveLocalInstance()
{
	if (!SkelotWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("SkelotWorld is not valid. Returning early..."));
		return;
	}

	if (LocalInstancesData.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Local Instance Map is empty. Returning early..."));
		return;
	}

	FLocalInstanceData& LocalInstanceData = LocalInstancesData[0];

	SkelotWorld->DestroyInstance(LocalInstanceData.InstanceHandle);

	LocalInstanceCounter--;

	UE_LOG(LogTemp, Log, TEXT("Removed Local Instance: %s"), *LocalInstanceData.UUID);
	LocalInstancesUUIDTracking.Remove(LocalInstanceData.UUID);
	LocalInstancesData.RemoveAt(0);
}

void ASkelotInstanceManager::RemoveAllLocalInstances()
{
	if (!SkelotWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("SkelotWorld is not valid. Returning early..."));
		return;
	}

	if (LocalInstancesData.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Local Instance Map is empty. Returning early..."));
		return;
	}

	for (const auto& LocalInstanceData : LocalInstancesData)
	{
		SkelotWorld->DestroyInstance(LocalInstanceData.InstanceHandle);
		UE_LOG(LogTemp, Log, TEXT("Removed Local Instance: %s"), *LocalInstanceData.UUID);
	}

	LocalInstancesData.Empty();
	LocalInstanceCounter = 0;
	UE_LOG(LogTemp, Log, TEXT("Removed all Local Instances"));
}

void ASkelotInstanceManager::StartActorUpdates()
{
	if (ActorUpdateTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ActorUpdateTimerHandle);
	}

	GetWorld()->GetTimerManager().SetTimer(ActorUpdateTimerHandle, this, &ASkelotInstanceManager::SendActorUpdates,
	                                       0.2f, true);
}

void ASkelotInstanceManager::StopActorUpdates()
{
	if (ActorUpdateTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(ActorUpdateTimerHandle);
	}
}

bool ASkelotInstanceManager::IsLocalInstance(const FString& UUID) const
{
	return LocalInstancesUUIDTracking.Contains(UUID);
}

void ASkelotInstanceManager::SendActorUpdates() const
{
	Async(EAsyncExecution::ThreadPool, [this]()
	{
		for (int32 i = 0; i < LocalInstanceCounter; i++)
		{
			const FLocalInstanceData& LocalInstanceData = LocalInstancesData[i];

			const FString UUIDtoSend = LocalInstanceData.UUID;

			FActorState ActorStateToDispatch;

			int64 ChunkX;
			int64 ChunkY;
			int64 ChunkZ;

			VoxelWorldControllerReference->CalculateChunkCoordinatesAtWorldLocation(
				LocalInstanceData.LocalActorState.Location, ChunkX, ChunkY, ChunkZ);

			const FVector ActorLocationCorrectionOffset = VoxelWorldControllerReference->
				CalculateChunkWorldPositionOrigin(ChunkX, ChunkY, ChunkZ);

			ActorStateToDispatch.Position = LocalInstanceData.LocalActorState.Location - ActorLocationCorrectionOffset;
			ActorStateToDispatch.Position.Z += InstanceConstants::STANDING_HEIGHT_OFFSET;
			ActorStateToDispatch.Rotation = LocalInstanceData.LocalActorState.Rotation;
			ActorStateToDispatch.Rotation.Yaw += InstanceConstants::ROTATION_YAW_OFFSET;
			ActorStateToDispatch.Velocity = LocalInstanceData.LocalActorState.Velocity;


			ActorServiceReference->SendActorUpdate(ChunkX, ChunkY, ChunkZ, UUIDtoSend, ActorStateToDispatch);
		}
	});
}


EMovementDirection ASkelotInstanceManager::GetMovementDirection(const FVector& FacingDirection,
                                                                const FVector& MovementDirection)
{
	const float Dot = FVector::DotProduct(FacingDirection, MovementDirection);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	const float CrossZ = FVector::CrossProduct(FacingDirection, MovementDirection).Z;

	const float SignedAngle = (CrossZ < 0) ? -AngleDeg : AngleDeg;
	const float Yaw360 = FMath::Fmod(SignedAngle + 360.0f, 360.0f);

	if (Yaw360 >= 337.5f || Yaw360 < 22.5f)
		return EMovementDirection::Forward;
	if (Yaw360 >= 22.5f && Yaw360 < 67.5f)
		return EMovementDirection::ForwardRight;
	if (Yaw360 >= 67.5f && Yaw360 < 112.5f)
		return EMovementDirection::Right;
	if (Yaw360 >= 112.5f && Yaw360 < 157.5f)
		return EMovementDirection::BackwardRight;
	if (Yaw360 >= 157.5f && Yaw360 < 202.5f)
		return EMovementDirection::Backward;
	if (Yaw360 >= 202.5f && Yaw360 < 247.5f)
		return EMovementDirection::BackwardLeft;
	if (Yaw360 >= 247.5f && Yaw360 < 292.5f)
		return EMovementDirection::Left;
	return EMovementDirection::ForwardLeft;
}

ERemoteAnimState ASkelotInstanceManager::SelectAnimationState(const float Speed, bool bIsCrouching,
                                                              const EMovementDirection Direction)
{
	const bool bIsIdle = Speed < 150.0f;
	const bool bIsRunning = Speed > 280.0f;

	if (bIsCrouching)
	{
		if (bIsIdle)
			return ERemoteAnimState::CrouchingIdle;

		switch (Direction)
		{
		case EMovementDirection::Forward: return ERemoteAnimState::CrouchingForward;
		case EMovementDirection::ForwardRight: return ERemoteAnimState::CrouchingForwardRight;
		case EMovementDirection::Right: return ERemoteAnimState::CrouchingRight;
		case EMovementDirection::BackwardRight: return ERemoteAnimState::CrouchingBackwardRight;
		case EMovementDirection::Backward: return ERemoteAnimState::CrouchingBackward;
		case EMovementDirection::BackwardLeft: return ERemoteAnimState::CrouchingBackwardLeft;
		case EMovementDirection::Left: return ERemoteAnimState::CrouchingLeft;
		case EMovementDirection::ForwardLeft: return ERemoteAnimState::CrouchingForwardLeft;
		}
	}
	else if (bIsIdle)
	{
		return ERemoteAnimState::Idle;
	}
	else if (bIsRunning)
	{
		switch (Direction)
		{
		case EMovementDirection::Forward: return ERemoteAnimState::Running;
		case EMovementDirection::ForwardRight: return ERemoteAnimState::RunningForwardRight;
		case EMovementDirection::Right: return ERemoteAnimState::RunningRight;
		case EMovementDirection::BackwardRight: return ERemoteAnimState::RunningBackwardRight;
		case EMovementDirection::Backward: return ERemoteAnimState::RunningBackward;
		case EMovementDirection::BackwardLeft: return ERemoteAnimState::RunningBackwardLeft;
		case EMovementDirection::Left: return ERemoteAnimState::RunningLeft;
		case EMovementDirection::ForwardLeft: return ERemoteAnimState::RunningForwardLeft;
		}
	}
	else // Walking
	{
		switch (Direction)
		{
		case EMovementDirection::Forward: return ERemoteAnimState::Walking;
		case EMovementDirection::ForwardRight: return ERemoteAnimState::WalkingForwardRight;
		case EMovementDirection::Right: return ERemoteAnimState::WalkingRight;
		case EMovementDirection::BackwardRight: return ERemoteAnimState::WalkingBackwardRight;
		case EMovementDirection::Backward: return ERemoteAnimState::WalkingBackward;
		case EMovementDirection::BackwardLeft: return ERemoteAnimState::WalkingBackwardLeft;
		case EMovementDirection::Left: return ERemoteAnimState::WalkingLeft;
		case EMovementDirection::ForwardLeft: return ERemoteAnimState::WalkingForwardLeft;
		}
	}

	return ERemoteAnimState::Idle; // fallback
}
