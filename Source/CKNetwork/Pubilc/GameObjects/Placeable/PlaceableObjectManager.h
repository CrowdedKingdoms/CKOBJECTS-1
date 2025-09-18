#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Voxels/Core/VoxelChunk.h"
#include "Shared/Types/Interfaces/World/OriginRebasable.h"
#include "PlaceableDefs.h"
#include "PlaceableObjectManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPlaceableManager, Log, All);

/**
 * Central manager you place in the level (or spawn at runtime).
 * - Holds a reference to a UGameObjectCatalog you edit in the editor.
 * - Offers BP functions to list entries and spawn them.
 * - Pools HISM components per StaticMesh for efficient instancing.
 */
UCLASS(Blueprintable)
class CROWDEDKINGDOMS_API APlaceableObjectManager : public AActor
{
	GENERATED_BODY()

public:
	APlaceableObjectManager();

	/** Assign your catalog asset here in the editor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeables")
	UGameObjectCatalog* Catalog;

	/** Rebuilds the ID->definition map (call after editing Catalog at runtime) */
	UFUNCTION(BlueprintCallable, Category = "Placeables")
	void RebuildRegistry();

	/** Returns UI-ready listing of all objects (IDs, names, thumbs, type flags) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Placeables")
	void GetAllObjectsUI(TArray<FGameObjectUIEntry>& OutList) const;

	/**
	 * Spawn (place) an object by ID at a transform.
	 * - For Actor entries: returns the spawned actor; OutInstanceIndex = -1, OutHISM = null.
	 * - For StaticMesh entries: returns nullptr; fills OutHISM + OutInstanceIndex.
	 */
	UFUNCTION(BlueprintCallable, Category = "Placeables")
	AActor* SpawnById(FName Id, const FTransform& Transform, AVoxelChunk* OwningChunk,
		/*out*/ UInstancedStaticMeshComponent*& OutHISM,
		/*out*/ int32& OutInstanceIndex);

	/** Removes a previously added instance from a given HISM (for undo/delete in UI) */
	UFUNCTION(BlueprintCallable, Category = "Placeables")
	bool RemoveInstance(UInstancedStaticMeshComponent* HISM, int32 InstanceIndex);

protected:
	virtual void BeginPlay() override;

	/** Map for quick lookup: Id -> Def */
	UPROPERTY(VisibleAnywhere, Category = "Placeables")
	TMap<FName, UGameObjectDef*> Registry;

	/** Pool of HISMs per mesh (so all instances of the same mesh share one component) */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> MeshToHISM;

	/** Root scene so our components have a stable parent */
	UPROPERTY()
	USceneComponent* Root;

	/** Creates or finds the pooled HISM for a given mesh */
	UInstancedStaticMeshComponent* GetOrCreateHISM(UStaticMesh* Mesh, bool bEnableCollision, AVoxelChunk* Chunk);

	/** Internal: spawn actor from actor def */
	AActor* SpawnActorFromDef(UGameObjectActorDef* ActorDef, const FTransform& Transform, AVoxelChunk* Chunk) const;
};
