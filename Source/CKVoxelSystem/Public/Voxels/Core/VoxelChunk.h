// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CKTypes/Public/Shared/Types/Structures/Voxels/FVoxelListItem.h"
#include "CKSharedTypes/Public/Shared/Types/Structures/Voxels/FVoxelState.h"
#include "CKSharedTypes/Public/Shared/Types/Structures/Voxels/FChunkVoxelState.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "CKTypes/Public/Shared/Types/Interfaces/World/OriginRebasable.h"
#include "CKTypes/Public/Shared/Types/Structures/Chunks/FChunkDataContainer.h"
#include "CKTypes/Public/Shared/Types/Structures/Voxels/FVoxelCoordinate.h"
#include "CKTypes/Public/Shared/Types/Structures/Voxels/FVoxelDefinition.h"
#include "VoxelChunk.generated.h"

class AAtlasManager;
class AVLOMeshProvider;


DECLARE_LOG_CATEGORY_EXTERN(LogVoxelChunk, Log, All);



UCLASS(Blueprintable, BlueprintType)
class AVoxelChunk : public AActor, public IOriginRebasable
{
	GENERATED_BODY()
	
public:
	AVoxelChunk();

	UFUNCTION()
	virtual FVector GetActorOriginalLocation_Implementation() override;
	
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void Initialize(int64 CX, int64 CY, int64 CZ, const float voxelSize, const int chunkSize);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetChunkHidden(bool hidden);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetStale(bool stale);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsStale() const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsChunkHidden() const;

	// Chebyshev distance from this chunk to chunkPos
	int DistanceToChunk(FInt64Vector chunkPos) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateChunk(const TArray<uint8>& voxels);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateVoxel(const int x, const int y, const int z, const uint8 voxelType);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateVoxelWithState(const int x, const int y, const int z, const uint8 voxelType, const FVoxelState& State);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateStates(const TArray<FChunkVoxelState>& states);
	
	void UpdateStates(const TMap<FVoxelCoordinate, FVoxelDefinition>& VoxelStatesMap);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateVoxels(const TArray<FVoxelListItem>& voxels);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateVoxelsAtlas(UTexture* TextureAtlas);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void UpdateVoxelAtWorldCoords(const FVector worldLocation, const uint8 voxelType);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void FillChunk(const uint8 voxelType);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	const TArray<uint8>& GetVoxelArray();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	const uint8 GetVoxel(const int x, const int y, const int z);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void ToggleChunkBounds(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void AddChunkAddressBillboard();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void RemoveChunkAddressBillboard();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void AddVoxelBillboards();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void RemoveVoxelBillboards();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	float GetOcclusionLvl() const;

	UFUNCTION(Category = "Voxel")
	void ApplyVoxelUpdates(const FChunkDataContainer& VoxelUpdateData);
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int64 X;
	int64 Y;
	int64 Z;

	UFUNCTION(BlueprintCallable, Category= "Voxel")
	void RotateVoxel(const int x, const int y, const int z);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void CycleFaceOneDirection(const int x, const int y, const int z);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void ChangeAtlasType(const int x, const int y, const int z, const int64 atlasType);
	
	void UpdateVoxelState(const int x, const int y, const int z, const FVoxelState& State);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FVoxelState& GetVoxelState(const int x, const int y, const int z);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsNonStandardVoxel(const int x, const int y, const int z);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetAtlasManagerReference(AAtlasManager* InAtlasManger);

    static void GenerateCubeMesh(FVector Position, FVector HalfSize, float VoxelSize, const bool VisibleFaces[6], TArray<FVector>& Vertices, TArray<int32>& Triangles,
                                 TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexOffset, int32& culledFaces, TArray<FLinearColor>& VertexColors, float VoxelTypeIndex, FVoxelState
                                 & State, TArray<FProcMeshTangent>& Tangents);


	UFUNCTION(BlueprintCallable, Category = "Voxel Chunk | VLO")
	void SetVLOMeshProvider(AVLOMeshProvider* InVLOMeshProvider);

	UFUNCTION(BlueprintCallable, Category = "Voxel Chunk | VLO")
	AVLOMeshProvider* GetVLOMeshProvider() const;

	UFUNCTION(BlueprintCallable, Category = "Voxel Chunk | VLO")
	bool IsVLOAtPosition(const int x, const int y, const int z) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	FVector GetVoxelCenter(const FVector VoxelPosition) const;

	UPROPERTY()
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> ObjectInstancedMeshes;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void DrawChunkBounds();

	UPROPERTY()
	UProceduralMeshComponent* mesh;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicColorMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Chunk | VLO")
	AVLOMeshProvider* VLOMeshProvider;

	UPROPERTY()
	TMap<uint8, UInstancedStaticMeshComponent*> VLOInstancedMeshes;

	void ClearAllVLOInstances();

	UInstancedStaticMeshComponent* GetOrCreateVLOComponent(uint8 VoxelType, UStaticMesh* Mesh);

	float OcclusionLevel = 0.0F;
	float SingleVoxelOcclusion = 0.0F;

private:
	void RegenerateChunk();
	
	float VoxelSize = 0.0F;
	uint32 ChunkSize = 0;
	uint32 NumOfVoxels = 0;
	TArray<uint8> Voxels;

	UPROPERTY()
	UTextRenderComponent* ChunkAddressText = nullptr;
	UPROPERTY()
	TArray<UTextRenderComponent*> VoxelBillboards;

	bool bDrawBounds = false;

	bool bInitialized = false;
	bool bHidden = false;
	bool bStale = false;

	UPROPERTY()
	TArray<FVoxelState> VoxelStates;

	static constexpr int32 ATLAS_COLUMNS = 6;
	static constexpr int32 ATLAS_ROWS = 255;
	static constexpr float ATLAS_UV_WIDTH = 1.0f/ATLAS_COLUMNS;
	static constexpr float ATLAS_UV_HEIGHT = 1.0f/ATLAS_ROWS;

	static void RotateUVs(int32 Rotation, FVector2D& UV0, FVector2D& UV1, FVector2D& UV2, FVector2D& UV3);
	static void DetermineVoxelFaceRotations(const uint8& FaceOneDirection, const uint8& Rotation, const int32& FaceIndex, int32& RotationAngle);
	static void DetermineVoxelFaces(const uint8& FaceOneDirection, const uint8& Rotation, const int32& FaceIndex, float& AtlasFace);
	UMaterialInstanceDynamic* CreateNewMaterialInstance(UTexture* InTexture);

	UFUNCTION()
	void HandleAtlasOverrides(UTexture* inAtlas);

	/**
	 * Determines if a voxel should be treated as transparent for face culling purposes.
	 * VLO voxels and air voxels are considered transparent.
	 * @param Vx X coordinate of the voxel
	 * @param Vy Y coordinate of the voxel  
	 * @param Vz Z coordinate of the voxel
	 * @return true if the voxel should be treated as transparent for culling, false otherwise
	 */

	bool IsVoxelTransparentForCulling(const uint32 Vx, const uint32 Vy, const uint32 Vz);
	
	/**
	 * Calculates the rotation for a VLO based on face direction and rotation value.
	 * @param FaceOneDirection Which direction the mesh should point - 0=+Z, 1=-X, 2=+Y, 3=+X, 4=-Y, 5=-Z
	 * @param Rotation Additional yaw rotation (0-3, each step is 90 degrees: 0°, 90°, 180°, 270°)
	 * @return The calculated rotation for the VLO mesh
	 */
	FRotator CalculateVLORotation(uint8 FaceOneDirection, uint8 Rotation) const;


	/**
	* Calculates the adjusted position for a VLO to keep it within voxel bounds when rotated.
	* @param BasePosition The original voxel center position
	* @param FaceOneDirection Which direction the mesh's Z vector should point
	* @param Rotation Additional yaw rotation value
	* @return The adjusted position for the VLO mesh
	*/
	FVector CalculateVLOPosition(const FVector& BasePosition, uint8 FaceOneDirection, uint8 Rotation) const;

	
	UPROPERTY(EditAnywhere, Category = "Voxel")
	AAtlasManager* AtlasManager = nullptr;

	UPROPERTY()
	UMaterialInterface* MaterialInstance;

	UPROPERTY()
	TArray<FVector> CalculatedNormals;

	UPROPERTY()
	TArray<FProcMeshTangent> CalculatedTangents;

	UPROPERTY()
	FVector OriginalLocation;
};
