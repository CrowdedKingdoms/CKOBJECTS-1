// Fill out your copyright notice in the Description page of Project Settings.


#include "Voxels/Rendering/VoxelMeshProvider.h"
#include "Voxels/Core/VoxelChunk.h"

DEFINE_LOG_CATEGORY(LogVoxelMeshProvider);


// Sets default values
UVoxelMeshProvider::UVoxelMeshProvider()
{
}

void UVoxelMeshProvider::GetVoxelMesh(uint8 VoxelType, UProceduralMeshComponent* VoxelMesh, UMaterialInstanceDynamic* MaterialInstance, float VoxelSize)
{
    FVector VoxelPosition = FVector(0.0f);
    FVector HalfSize = FVector(VoxelSize / 2.0f);

    bool VisibleFaces[6] = { true, true, true, true, true, true };  // Render all faces
    int32 CulledFaces = 0;

    struct FMeshData
    {
        TArray<FVector> Vertices;
        TArray<FLinearColor> VertexColors;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        int32 VertexOffset = 0;
        TArray<FProcMeshTangent> Tangents;
        UMaterialInstanceDynamic* AtlasMaterial;
    };

    FMeshData MeshData;
    FVoxelState State;

    // Calculate voxel type index (used for texture atlas or other purposes)
    UEnum* EnumPtr = StaticEnum<EVoxelType>();
    float VoxelTypeIndex = static_cast<float>(VoxelType) / static_cast<float>(EnumPtr->GetMaxEnumValue() - 1);

    // Reusing GenerateCubeMesh from VoxelChunk to handle mesh generation
    AVoxelChunk::GenerateCubeMesh(VoxelPosition, HalfSize, VoxelSize, VisibleFaces, MeshData.Vertices, MeshData.Triangles,
        MeshData.Normals, MeshData.UVs, MeshData.VertexOffset, CulledFaces, MeshData.VertexColors, VoxelTypeIndex, State, MeshData.Tangents);

    // UE_LOG(LogVoxelMeshProvider, Log, TEXT("Mesh vertices count: %d"), MeshData.Vertices.Num());

    // Assign generated mesh to the procedural mesh component
    VoxelMesh->CreateMeshSection_LinearColor(0, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UVs, MeshData.VertexColors, MeshData.Tangents, true);
    VoxelMesh->SetMaterial(0, MaterialInstance);
}
