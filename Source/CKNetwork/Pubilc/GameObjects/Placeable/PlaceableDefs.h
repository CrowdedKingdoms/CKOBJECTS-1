#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "PlaceableDefs.generated.h"

/** UI record you can pass to Blueprints for menus/thumbnails */
USTRUCT(BlueprintType)
struct FGameObjectUIEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameObject")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameObject")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameObject")
	UTexture2D* Thumbnail = nullptr;

	/** True if this entry spawns an Actor class, false if it's a StaticMesh instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameObject")
	bool bIsActor = false;
};

/** Base definition for a placeable game object */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class UGameObjectDef : public UObject
{
	GENERATED_BODY()

public:
	/** Stable, human-friendly unique ID (required) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameObject")
	FName UniqueId;

	/** Display name for UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameObject")
	FText DisplayName;

	/** Optional thumbnail for UI (soft so it can stream) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameObject")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	/** By default, entries are valid only if they have an ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameObject")
	virtual bool IsValidDef() const { return !UniqueId.IsNone(); }

	/** Helper: resolve (load) thumbnail synchronously for UI (you can async this later) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GameObject")
	UTexture2D* ResolveThumbnailSync() const { return Thumbnail.IsNull() ? nullptr : Thumbnail.LoadSynchronous(); }
};

/** Static mesh entry. Will be spawned as HISM instance(s). */
UCLASS(BlueprintType, EditInlineNew)
class UGameObjectStaticMeshDef : public UGameObjectDef
{
	GENERATED_BODY()

public:
	/** The mesh to render (uses its default materials) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StaticMesh")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Optional: collision on/off for the pooled HISM */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StaticMesh")
	bool bEnableCollision = true;

	virtual bool IsValidDef() const override
	{
		return Super::IsValidDef() && !Mesh.IsNull();
	}

	/** Helper: resolve (load) mesh synchronously */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "StaticMesh")
	UStaticMesh* ResolveMeshSync() const
	{
		return Mesh.IsNull() ? nullptr : Mesh.LoadSynchronous();
	}
};

/** Actor entry. Will spawn an independent actor (BP or C++). */
UCLASS(BlueprintType, EditInlineNew)
class UGameObjectActorDef : public UGameObjectDef
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor")
	TSoftClassPtr<AActor> ActorClass;

	/** Whether the spawned actor should replicate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actor")
	bool bReplicates = false;

	virtual bool IsValidDef() const override
	{
		return Super::IsValidDef() && !ActorClass.IsNull();
	}

	/** Helper: resolve (load) actor class synchronously */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Actor")
	UClass* ResolveActorClassSync() const
	{
		return ActorClass.IsNull() ? nullptr : ActorClass.LoadSynchronous();
	}
};

/** A BP-editable catalog you can fill with any number of entries */
UCLASS(BlueprintType)
class UGameObjectCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// You can override this if you want a type name:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("GameObjectCatalog", GetFName());
	}

	/** Inline/instanced so you can add rows right in the asset Details panel */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Catalog")
	TArray<UGameObjectDef*> Entries;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Catalog")
	UGameObjectDef* FindById(FName Id) const
	{
		for (UGameObjectDef* Def : Entries)
		{
			if (Def && Def->UniqueId == Id)
			{
				return Def;
			}
		}
		return nullptr;
	}

protected:
	// Called after the asset is loaded
	virtual void PostLoad() override
	{
		Super::PostLoad();

		// Log which catalog was loaded and what primary asset ID UE assigned
		UE_LOG(LogTemp, Log, TEXT("UGameObjectCatalog loaded: %s (PrimaryAssetId: %s)"),
			*GetName(), *GetPrimaryAssetId().ToString());

		// Optionally list all entry IDs for debug
		for (UGameObjectDef* Def : Entries)
		{
			if (Def)
			{
				UE_LOG(LogTemp, Log, TEXT(" - Entry: %s"), *Def->UniqueId.ToString());
			}
		}
	}
};
