// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/NonAuthClients/NPC_Manager.h"
#include "Async/Async.h"
#include "GameFramework/Character.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NonAuthClients/SkelotInstanceManager.h"
#include "Voxels/Core/VoxelWorldSubsystem.h"

// Sets default values
ANPC_Manager::ANPC_Manager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ANPC_Manager::BeginPlay()
{
	Super::BeginPlay();

	OriginalLocation = GetActorLocation();

	StartTimeoutCheckingTask();
}

void ANPC_Manager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	bShouldStopTimeoutTask = true;
}

// Main Processing Functions (called from other classes)
void ANPC_Manager::ProcessUpdates(const FActorUpdateStruct& UpdateInfo)
{
	// Processing Information
	const int64 ChunkX = UpdateInfo.ChunkX;
	const int64 ChunkY = UpdateInfo.ChunkY;
	const int64 ChunkZ = UpdateInfo.ChunkZ;
	const FString UUID = UpdateInfo.UUID;
	FActorState receivedActorState = UpdateInfo.State;

	// We do nothing if it's owner UUID.
	if (OwnerUUID == UUID)
	{
		if (!bPlayerGhostEnabled)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "This is Owner UUID. Ignoring Update.");
			return;
		}
	}

	if (SkelotInstanceManager->IsLocalInstance(UUID))
	{
		return; // Do not process updates for locally spawned instances.
	}

	// Update the last-seen time for this UUID's Update
	UpdateLastSeenTime(UUID);

	if (CheckPlayerExists(UUID)) // Update the player if it exists
	{
		UpdatePlayer(UUID, ChunkX, ChunkY, ChunkZ, receivedActorState);
		//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "This Player UUID Already exists.");
	}
	else
	{
		RegisterPlayerToMap(UUID);
		SpawnNewPlayer(UUID, ChunkX, ChunkY, ChunkZ, receivedActorState);
		//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "New UUID. New Player Added to List.")
	}
}


FVector ANPC_Manager::GetActorOriginalLocation_Implementation()
{
	return OriginalLocation;
}

void ANPC_Manager::StartTimeoutCheckingTask()
{
	bShouldStopTimeoutTask = false;

    // Use a dedicated OS thread for periodic timeout checks to avoid task graph starvation in packaged builds
    Async(EAsyncExecution::Thread, [this]()
    {
        while (!bShouldStopTimeoutTask && IsValid(this))
        {
            FPlatformProcess::Sleep(5.0);

            if (!IsValid(this) || bShouldStopTimeoutTask)
            {
                break;
            }

            CheckForActorTimeouts();
        }
    });
}

// Update Timeout Related Functions
void ANPC_Manager::CheckForActorTimeouts()
{
	const float CurrentTime = FPlatformTime::Seconds();
	TArray<FString> TimedOutActorUUIDS;


	for (const FString& UUID : OtherPlayersUUID)
	{
		if (const float* LastUpdateTime = LastUpdateTimes.Find(UUID))
		{
			if ((CurrentTime - *LastUpdateTime) >= ActorTimeoutThreshold)
			{
				TimedOutActorUUIDS.Add(UUID);
			}
		}
	}


	for (const FString& UUID : TimedOutActorUUIDS)
	{
		LastUpdateTimes.Remove(UUID);
		OtherPlayersUUID.Remove(UUID);
	}


	if (TimedOutActorUUIDS.Num() > 0)
	{
		ProcessTimedOutActor(TimedOutActorUUIDS);
	}
}

void ANPC_Manager::UpdateLastSeenTime(const FString& UUID)
{
	FScopeLock Lock(&TimeoutMapLock);
	LastUpdateTimes.Add(UUID, FPlatformTime::Seconds());
}


// Actor Related Functions
bool ANPC_Manager::CheckPlayerExists(const FString& IncomingUUID) const
{
	return OtherPlayersUUID.Contains(IncomingUUID);
}

void ANPC_Manager::RegisterPlayerToMap(const FString& IncomingUUID)
{
	FScopeLock Lock(&UUIDLock);
	OtherPlayersUUID.Add(IncomingUUID);
}

void ANPC_Manager::SpawnNewPlayer(const FString& InComingUUID, const int64& ChunkX, const int64& ChunkY,
                                  const int64& ChunkZ, FActorState& ActorState)
{
	ActorState.Position = ActorState.Position + VoxelWorldController->CalculateChunkWorldPositionOrigin(
		ChunkX, ChunkY, ChunkZ);

	SkelotInstanceManager->AddInstance(InComingUUID, ActorState);

	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, InComingUUID, ChunkX, ChunkY, ChunkZ, ActorState]()
	{
		SpawnNPC.Broadcast(InComingUUID, ActorState.Position);
	}, UE::Tasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}

void ANPC_Manager::UpdatePlayer(const FString& InComingUUID, const int64& ChunkX, const int64& ChunkY,
                                const int64& ChunkZ, FActorState& ActorState) const
{
	ActorState.Position = ActorState.Position + VoxelWorldController->CalculateChunkWorldPositionOrigin(
		ChunkX, ChunkY, ChunkZ);

	SkelotInstanceManager->UpdateInstance(InComingUUID, ActorState);
	
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, InComingUUID, ChunkX, ChunkY, ChunkZ, ActorState]()
	{
		UpdateNPC.Broadcast(InComingUUID, ActorState.Position);
		
	}, UE::Tasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}

void ANPC_Manager::SetVoxelWorldController(UVoxelWorldSubsystem* WorldController)
{
	VoxelWorldController = WorldController;
	SkelotInstanceManager->SetVoxelControllerReference(WorldController);
}

void ANPC_Manager::ProcessTimedOutActor(const TArray<FString>& TimedOutActors) const
{
	if (!IsValid(this))
	{
		return;
	}

	UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, TimedOutActors]()
	{
		for (int32 Index = 0; Index < TimedOutActors.Num(); Index++)
		{
			ActorTimeout.Broadcast(TimedOutActors[Index]);
			SkelotInstanceManager->RemoveInstance(TimedOutActors[Index]);
			UE_LOG(LogTemp, Log, TEXT("Actor %s timed out"), *TimedOutActors[Index]);
		}
	}, LowLevelTasks::ETaskPriority::Normal, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);

}
