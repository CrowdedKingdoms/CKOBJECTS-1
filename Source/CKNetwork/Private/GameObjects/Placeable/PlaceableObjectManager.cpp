#include "CKNetwork/Pubilc/GameObjects/Placeable/PlaceableObjectManager.h"
#include "CKVoxelSystem/Public/Voxels/Core/VoxelChunk.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogPlaceableManager);

APlaceableObjectManager::APlaceableObjectManager()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Keep the manager invisible & cheap
	SetActorHiddenInGame(true);
	SetReplicates(false);
}

void APlaceableObjectManager::BeginPlay()
{
	Super::BeginPlay();
	RebuildRegistry();
}

void APlaceableObjectManager::RebuildRegistry()
{
	Registry.Empty();

	if (!Catalog)
	{
		UE_LOG(LogPlaceableManager, Warning, TEXT("No Catalog set on %s"), *GetName());
		return;
	}

	for (UGameObjectDef* Def : Catalog->Entries)
	{
		if (!IsValid(Def) || !Def->IsValidDef())
		{
			if (Def)
			{
				UE_LOG(LogPlaceableManager, Warning, TEXT("Invalid entry in catalog: %s"), *Def->GetName());
			}
			continue;
		}

		if (Registry.Contains(Def->UniqueId))
		{
			UE_LOG(LogPlaceableManager, Warning, TEXT("Duplicate UniqueId '%s' in catalog; keeping the first one."),
				*Def->UniqueId.ToString());
			continue;
		}

		Registry.Add(Def->UniqueId, Def);
	}

	UE_LOG(LogPlaceableManager, Log, TEXT("Rebuilt registry: %d entries"), Registry.Num());
}

void APlaceableObjectManager::GetAllObjectsUI(TArray<FGameObjectUIEntry>& OutList) const
{
	OutList.Reset();

	// Iterate deterministic for stable UI ordering (optional)
	TArray<FName> Keys;
	Registry.GetKeys(Keys);
	Keys.Sort(FNameLexicalLess());

	for (const FName& Id : Keys)
	{
		UGameObjectDef* Entry = Registry[Id];
		FGameObjectUIEntry Row;
		Row.Id = Entry->UniqueId;
		Row.DisplayName = Entry->DisplayName;
		Row.Thumbnail = Entry->ResolveThumbnailSync();

		const bool bIsActor =
			Cast<UGameObjectActorDef>(Entry) != nullptr;

		Row.bIsActor = bIsActor;

		OutList.Add(Row);
	}
}

UInstancedStaticMeshComponent* APlaceableObjectManager::GetOrCreateHISM(UStaticMesh* Mesh, bool bEnableCollision, AVoxelChunk* Chunk = nullptr)
{
	if (!Mesh) return nullptr;

	bool bChunkOwned = (Chunk != nullptr);

	// Use the correct map
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Map =
		bChunkOwned ? Chunk->ObjectInstancedMeshes : this->MeshToHISM;

	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = Map.Find(Mesh))
	{
		return *Found;
	}

	// Create new component
	UObject* Outer = bChunkOwned ? (UObject*)Chunk : (UObject*)this;
	UInstancedStaticMeshComponent* NewComponent = NewObject<UInstancedStaticMeshComponent>(
		Outer,
		UInstancedStaticMeshComponent::StaticClass(),
		*FString::Printf(TEXT("ISM_%s"), *Mesh->GetName())
	);

	if (!NewComponent)
	{
		UE_LOG(LogPlaceableManager, Error, TEXT("Failed to create Instanced static mesh component for '%s'"), *Mesh->GetName());
		return nullptr;
	}

	NewComponent->SetStaticMesh(Mesh);
	NewComponent->SetVisibility(true);
	NewComponent->SetHiddenInGame(false);
	NewComponent->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	NewComponent->SetCastShadow(true);

	// Attach to appropriate root
	USceneComponent* ParentRoot = bChunkOwned ? Chunk->GetRootComponent() : (USceneComponent*)RootComponent;
	NewComponent->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepWorldTransform);
	NewComponent->RegisterComponent();
	Map.Add(Mesh, NewComponent);
	UE_LOG(LogPlaceableManager, Log, TEXT("Successfully created and registered static mesh component for '%s'"), *Mesh->GetName());

	return NewComponent;
}

AActor* APlaceableObjectManager::SpawnActorFromDef(UGameObjectActorDef* ActorDef, const FTransform& Transform, AVoxelChunk* Chunk = nullptr) const
{
	if (!ActorDef) return nullptr;

	TSubclassOf<AActor> ClassToSpawn = ActorDef->ActorClass.LoadSynchronous();
	if (!*ClassToSpawn)
	{
		UE_LOG(LogPlaceableManager, Error, TEXT("Actor class not loaded for %s"), *ActorDef->GetName());
		return nullptr;
	}

	FActorSpawnParameters Params; 
	Params.Owner = Chunk ? (AActor*)Chunk : (AActor*)this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Spawned = GetWorld()->SpawnActor<AActor>(ClassToSpawn, Transform, Params);
	if (!Spawned)
	{
		UE_LOG(LogPlaceableManager, Error, TEXT("Actor not spawned for %s"), *ActorDef->GetName());
		return nullptr;
	}

	if (Chunk)
	{
		Spawned->AttachToActor(Chunk, FAttachmentTransformRules::KeepWorldTransform);
	}

	Spawned->SetReplicates(ActorDef->bReplicates);
	Spawned->SetReplicateMovement(ActorDef->bReplicates);
	return Spawned;
}

AActor* APlaceableObjectManager::SpawnById(FName Id, const FTransform& Transform, AVoxelChunk* OwningChunk,
	UInstancedStaticMeshComponent*& OutHISM,
	int32& OutInstanceIndex)
{
	OutHISM = nullptr;
	OutInstanceIndex = INDEX_NONE;

	UGameObjectDef** Found = Registry.Find(Id);
	if (!Found || !*Found)
	{
		UE_LOG(LogPlaceableManager, Warning, TEXT("SpawnById: Unknown Id '%s'"), *Id.ToString());
		return nullptr;
	}

	// Static mesh route (instancing)
	if (UGameObjectStaticMeshDef* SMDef = Cast<UGameObjectStaticMeshDef>(*Found))
	{
		UStaticMesh* Mesh = SMDef->Mesh.LoadSynchronous();
		if (!Mesh)
		{
			UE_LOG(LogPlaceableManager, Error, TEXT("SpawnById: Mesh failed to load for '%s'"), *Id.ToString());
			return nullptr;
		}

		UE_LOG(LogPlaceableManager, Log, TEXT("Mesh '%s' has %d materials, LODs: %d"),
			*Mesh->GetName(), Mesh->GetStaticMaterials().Num(), Mesh->GetNumLODs());

		UInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Mesh, SMDef->bEnableCollision, OwningChunk);
		if (!HISM)
		{
			UE_LOG(LogPlaceableManager, Error, TEXT("SpawnById: Failed to get or create HISM for '%s'"), *Id.ToString());
			return nullptr;
		}

		OutInstanceIndex = HISM->AddInstance(Transform);
		OutHISM = HISM;

		// For instanced meshes we do NOT return an actor; the manager owns the component.
		return nullptr;
	}

	// Actor route
	if (UGameObjectActorDef* ADef = Cast<UGameObjectActorDef>(*Found))
	{
		return SpawnActorFromDef(ADef, Transform, OwningChunk);
	}

	UE_LOG(LogPlaceableManager, Warning, TEXT("SpawnById: Id '%s' has unsupported def type"), *Id.ToString());
	return nullptr;
}

bool APlaceableObjectManager::RemoveInstance(UInstancedStaticMeshComponent* HISM, int32 InstanceIndex)
{
	if (!HISM || InstanceIndex < 0) return false;
	if (!HISM->IsRegistered())
	{
		// If not registered yet, mark for deferred removal; simplest path is guard:
		return false;
	}

	// RemoveInstance compacts the array; track indices externally if you need stable IDs.
	return HISM->RemoveInstance(InstanceIndex);
}
