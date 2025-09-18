// Fill out your copyright notice in the Description page of Project Settings.

#include "Voxels/Core/VoxelChunk.h"
#include "CKNetwork/Pubilc/GameObjects/Placeable/PlaceableObjectManager.h"
#include "Containers/Array.h"
#include "Voxels/Rendering/AtlasManager.h"
#include "Shared/Types/Structures/Voxels/FChunkVoxelState.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Shared/Types/Enums/Voxels/EVoxelType.h"
#include "Voxels/Rendering/VLOMeshProvider.h"

DEFINE_LOG_CATEGORY(LogVoxelChunk);

// Sets default values
AVoxelChunk::AVoxelChunk()
{
	PrimaryActorTick.bCanEverTick = false;
    bInitialized = false;

    //mesh->bUseComplexAsSimpleCollision = true;
    //mesh->SetMobility(EComponentMobility::Movable);
    //mesh->SetVisibility(true);
    // 
    // Create the ProceduralMeshComponent
    mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedVoxelMesh"));
    RootComponent = mesh;
    PrimaryActorTick.SetTickFunctionEnable(false);
}

FVector AVoxelChunk::GetActorOriginalLocation_Implementation()
{
    return OriginalLocation;
}

void AVoxelChunk::Initialize(int64 CX, int64 CY, int64 CZ, const float voxelSize, const int chunkSize)
{
    X = CX;
    Y = CY;
    Z = CZ;

    VoxelSize = voxelSize;
    ChunkSize = static_cast<uint32>(chunkSize);
    NumOfVoxels = ChunkSize * ChunkSize * ChunkSize;

    SingleVoxelOcclusion = 1.0 / NumOfVoxels;

    Voxels.Init(0U, NumOfVoxels);
    OcclusionLevel = 0.0F;
    VoxelBillboards.Init(nullptr, NumOfVoxels);

    // Set this actor to call Tick() every frame.
    PrimaryActorTick.bCanEverTick = true;
    bInitialized = true;

    // Initializing Voxel States for all voxels in a chunk
    VoxelStates.Init(FVoxelState(), NumOfVoxels);

    OriginalLocation = FVector(CX * chunkSize * voxelSize, 
                                        CY * chunkSize * voxelSize, 
                                        CZ * chunkSize * voxelSize);
}

void AVoxelChunk::SetChunkHidden(bool hidden)
{
    SetActorHiddenInGame(hidden);
    bHidden = hidden;
}

void AVoxelChunk::SetStale(bool stale)
{
    bStale = stale;
}


// Called when the game starts or when spawned
void AVoxelChunk::BeginPlay()
{
	Super::BeginPlay();

    MaterialInstance = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Assets/Materials/MI_NewVoxelMaterial.MI_NewVoxelMaterial"));
    
	if (MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load material instance."));
	}

    OriginalLocation = GetActorLocation();
    
}

void AVoxelChunk::DrawChunkBounds()
{
    FVector ChunkCenter = GetActorLocation();
    FVector Extent = FVector(ChunkSize * VoxelSize / 2.0F);
    DrawDebugBox(GetWorld(), ChunkCenter, Extent, FColor::Green);
}

// Called every frame
void AVoxelChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (bDrawBounds)
    {
        DrawChunkBounds();
    }
    
    /*
    // Rotate any billboards toward the camera
    if (ChunkAddressText != nullptr || VoxelBillboards.Num() > 0)
    {
        // Get the player's camera location and rotation
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController)
        {
            FVector CameraLocation;
            FRotator CameraRotation;
            PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

            // Ensure Text Component is valid
            if (ChunkAddressText != nullptr)
            {
                // Find the rotation needed to look at the camera
                FVector TextLocation = ChunkAddressText->GetComponentLocation();
                FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(TextLocation, CameraLocation);

                // Apply the rotation to the text render component
                ChunkAddressText->SetWorldRotation(LookAtRotation);
            }

            for (auto* Billboard : VoxelBillboards)
            {
                if (Billboard != nullptr)
                {
                    // Find the rotation needed to look at the camera
                    FVector BillboardLocation = Billboard->GetComponentLocation();
                    FRotator BillboardLookAtRotation = UKismetMathLibrary::FindLookAtRotation(BillboardLocation, CameraLocation);

                    // Apply the rotation to the text render component
                    Billboard->SetWorldRotation(BillboardLookAtRotation);
                }
            }
        }
    }
    */
}



bool AVoxelChunk::IsStale() const
{
    return bStale;
}

bool AVoxelChunk::IsChunkHidden() const
{
    return bHidden;
}

int AVoxelChunk::DistanceToChunk(FInt64Vector chunkPos) const
{
    return FMath::Max3(FMath::Abs(X - chunkPos.X), FMath::Abs(Y - chunkPos.Y), FMath::Abs(Z - chunkPos.Z));
}

void AVoxelChunk::UpdateChunk(const TArray<uint8>& voxels)
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateChunk called on uninitialized chunk"));
        return;
    }

    if (voxels.Num() != Voxels.Num())
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateChunk called with unexpected voxels size! Expected %d, got %d"), Voxels.Num(), voxels.Num());
        return;
    }

    if (voxels == Voxels)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateChunk called with identical voxels!"));
        return;
    }

    //UE_LOG(LogVoxelChunk, Log, TEXT("AVoxelChunk::UpdateChunk called"));

    constexpr uint8 EmptyVoxel = static_cast<uint8>(EVoxelType::AIR);
    uint32 NumOfEmptyVoxels = voxels.FilterByPredicate([EmptyVoxel](const uint8 voxel)
        {
            return voxel == EmptyVoxel;
        }).Num();

    OcclusionLevel = static_cast<float>(NumOfEmptyVoxels) / static_cast<float>(NumOfVoxels);

    Voxels = voxels;
    RegenerateChunk();
}

void AVoxelChunk::UpdateVoxel(const int x, const int y, const int z, const uint8 voxelType)
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateVoxel called on uninitialized chunk"));
        return;
    }

    uint32 voxelIndex{ x + y * ChunkSize + z * ChunkSize * ChunkSize };

    if (voxelIndex >= static_cast<uint32>(Voxels.Num()))
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateVoxel called with invalid coordinates (%d, %d, %d)"), x, y, z);
        return;
    }

    //UE_LOG(LogVoxelChunk, Log, TEXT("AVoxelChunk::UpdateVoxel(%d, %d, %d) called"), x, y, z);

    constexpr uint8 EmptyVoxel = static_cast<uint8>(EVoxelType::AIR);
    if ((Voxels[voxelIndex] == EmptyVoxel) && (voxelType != EmptyVoxel))
    {
        OcclusionLevel += SingleVoxelOcclusion;
    }
    else if ((Voxels[voxelIndex] != EmptyVoxel) && (voxelType == EmptyVoxel))
    {
        OcclusionLevel -= SingleVoxelOcclusion;
    }

    if (voxelType == static_cast<uint8>(EVoxelType::AIR))
    {
        VoxelStates[voxelIndex].Rotation = 0;
        VoxelStates[voxelIndex].AtlasOverride = 0;
        VoxelStates[voxelIndex].FaceOneDirection = 0;
        VoxelStates[voxelIndex].bIsVLO = false;
    }
    
    Voxels[voxelIndex] = voxelType;
    VoxelStates[voxelIndex].ResetVoxelState();
    RegenerateChunk();
}

void AVoxelChunk::UpdateVoxelWithState(const int x, const int y, const int z, const uint8 voxelType,
    const FVoxelState& State)
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateVoxel called on uninitialized chunk"));
        return;
    }

    uint32 voxelIndex{ x + y * ChunkSize + z * ChunkSize * ChunkSize };

    if (voxelIndex >= static_cast<uint32>(Voxels.Num()))
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateVoxel called with invalid coordinates (%d, %d, %d)"), x, y, z);
        return;
    }

    //UE_LOG(LogVoxelChunk, Log, TEXT("AVoxelChunk::UpdateVoxel(%d, %d, %d) called"), x, y, z);

    constexpr uint8 EmptyVoxel = static_cast<uint8>(EVoxelType::AIR);
    if ((Voxels[voxelIndex] == EmptyVoxel) && (voxelType != EmptyVoxel))
    {
        OcclusionLevel += SingleVoxelOcclusion;
    }
    else if ((Voxels[voxelIndex] != EmptyVoxel) && (voxelType == EmptyVoxel))
    {
        OcclusionLevel -= SingleVoxelOcclusion;
    }

    Voxels[voxelIndex] = voxelType;

    FVoxelState& CurrentVoxelState = GetVoxelState(x, y, z);
    CurrentVoxelState.Rotation = State.Rotation;
    CurrentVoxelState.AtlasOverride = State.AtlasOverride;
    CurrentVoxelState.FaceOneDirection = State.FaceOneDirection;
    CurrentVoxelState.bIsVLO = State.bIsVLO;
    CurrentVoxelState.GameObjects = State.GameObjects;

    RegenerateChunk();
}

void AVoxelChunk::UpdateStates(const TArray<FChunkVoxelState>& states)
{
    for (const auto& state : states)
    {
        uint32 VoxelStateIndex{state.Vx + state.Vy * ChunkSize + state.Vz * ChunkSize * ChunkSize};

        if (VoxelStateIndex >= static_cast<uint32>(Voxels.Num()))
        {
            continue;
        }

        VoxelStates[VoxelStateIndex] = state.VoxelState;
    }
}

void AVoxelChunk::UpdateStates(const TMap<FVoxelCoordinate, FVoxelDefinition>& VoxelStatesMap)
{
    bool bHasAnyChange = false;

    for (const TPair<FVoxelCoordinate, FVoxelDefinition>& Pair : VoxelStatesMap)
    {
        const FVoxelCoordinate& Coordinate = Pair.Key;
        const FVoxelDefinition& VoxelDef = Pair.Value;

        const uint32 VoxelStateIndex{Coordinate.X + Coordinate.Y * ChunkSize + Coordinate.Z * ChunkSize * ChunkSize};

        if (VoxelStateIndex >= static_cast<uint32>(Voxels.Num()))
        {
            continue;
        }

        // Early-diff check
        if (VoxelStates[VoxelStateIndex] != VoxelDef.VoxelState || Voxels[VoxelStateIndex] != VoxelDef.VoxelType)
        {
            bHasAnyChange = true;
            break;
        }
    }

    if (!bHasAnyChange)
    {
        // No actual state or voxel changes
        return;
    }

    // Perform the actual updates now
    for (const TPair<FVoxelCoordinate, FVoxelDefinition>& Pair : VoxelStatesMap)
    {
        const FVoxelCoordinate& Coordinate = Pair.Key;
        const FVoxelDefinition& VoxelDef = Pair.Value;

        const uint32 VoxelStateIndex{Coordinate.X + Coordinate.Y * ChunkSize + Coordinate.Z * ChunkSize * ChunkSize};

        if (VoxelStateIndex >= static_cast<uint32>(Voxels.Num()))
        {
            continue;
        }

        VoxelStates[VoxelStateIndex] = VoxelDef.VoxelState;
        Voxels[VoxelStateIndex] = VoxelDef.VoxelType;
    }

    RegenerateChunk();
}


void AVoxelChunk::UpdateVoxels(const TArray<FVoxelListItem>& voxels)
{
    for (const auto& voxel : voxels)
    {
        uint32 voxelIndex{ voxel.x + voxel.y * ChunkSize + voxel.z * ChunkSize * ChunkSize };

        if (voxelIndex >= static_cast<uint32>(Voxels.Num()))
        {
            UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateVoxels called with invalid coordinates (%d, %d, %d)"), voxel.x, voxel.y, voxel.z);
            continue;
        }

        constexpr uint8 EmptyVoxel = static_cast<uint8>(EVoxelType::AIR);
        if ((Voxels[voxelIndex] == EmptyVoxel) && (voxel.type != EmptyVoxel))
        {
            OcclusionLevel += SingleVoxelOcclusion;
        }
        else if ((Voxels[voxelIndex] != EmptyVoxel) && (voxel.type == EmptyVoxel))
        {
            OcclusionLevel -= SingleVoxelOcclusion;
        }

        Voxels[voxelIndex] = voxel.type;
    }

    RegenerateChunk();
}

void AVoxelChunk::UpdateVoxelsAtlas(UTexture* TextureAtlas)
{
	DynamicMaterialInstance->SetTextureParameterValue(FName("TextureAtlas"), TextureAtlas);
}

void AVoxelChunk::UpdateVoxelAtWorldCoords(const FVector worldLocation, const uint8 voxelType)
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::UpdateVoxelAtWorldCoords called on uninitialized chunk"));
        return;
    }

    int ChunkSizeUnrealUnits = ChunkSize * VoxelSize;

    int x = FMath::FloorToInt(worldLocation.X) % ChunkSizeUnrealUnits;
    if (x < 0) x += ChunkSizeUnrealUnits;

    int y = FMath::FloorToInt(worldLocation.Y) % ChunkSizeUnrealUnits;
    if (y < 0) y += ChunkSizeUnrealUnits;

    int z = FMath::FloorToInt(worldLocation.Z) % ChunkSizeUnrealUnits;
    if (z < 0) z += ChunkSizeUnrealUnits;

    UpdateVoxel(x / VoxelSize, y / VoxelSize, z / VoxelSize, voxelType);
}

void AVoxelChunk::FillChunk(const uint8 voxelType)
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::FillChunk called on uninitialized chunk"));
        return;
    }

    //UE_LOG(LogVoxelChunk, Log, TEXT("AVoxelChunk::FillChunk(%d) called"), voxelType);
    Voxels.Init(voxelType, Voxels.Num());

    if (voxelType == static_cast<uint8>(EVoxelType::AIR))
    {
        OcclusionLevel = 0.0F;
    }
    else
    {
        OcclusionLevel = 1.0F;
    }

    RegenerateChunk();
}

const TArray<uint8>& AVoxelChunk::GetVoxelArray()
{
    return Voxels;
}

const uint8 AVoxelChunk::GetVoxel(const int x, const int y, const int z)
{
    const int index = x + y * ChunkSize + z * ChunkSize * ChunkSize;

    // Return empty voxel if the index is out of bounds.
    if (index < 0 || index >= Voxels.Num())
    {
        UE_LOG(LogVoxelChunk, Error, TEXT("AVoxelChunk::GetVoxel called with invalid parameters(%d %d %d). Array size: %d"), x, y, z, Voxels.Num());
        return static_cast<uint8>(EVoxelType::AIR);
    }

    return Voxels[index];
}

void AVoxelChunk::ToggleChunkBounds(bool bEnable)
{
    bDrawBounds = bEnable;
}

void AVoxelChunk::AddChunkAddressBillboard()
{
    if (ChunkAddressText != nullptr)
    {
        RemoveChunkAddressBillboard();
    }

    FString strAddress = FString::Printf(TEXT("%lld, %lld, %lld"), X, Y, Z);

    ChunkAddressText = NewObject<UTextRenderComponent>(this);
    ChunkAddressText->RegisterComponent();
    ChunkAddressText->SetText(FText::FromString(strAddress));
    ChunkAddressText->SetWorldLocation(GetActorLocation());
    ChunkAddressText->SetWorldSize(50.F);
    ChunkAddressText->SetTextRenderColor(FColor::Green);
    ChunkAddressText->SetVisibility(true);
    ChunkAddressText->SetHorizontalAlignment(EHTA_Center);
    ChunkAddressText->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

    //UE_LOG(LogVoxelChunk, Log, TEXT("AVoxelChunk::AddChunkAddressBillboard called. Billboard txt: %s"), *strAddress);
}

void AVoxelChunk::RemoveChunkAddressBillboard()
{
    if (ChunkAddressText != nullptr)
    {
        ChunkAddressText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        ChunkAddressText->DestroyComponent();
        ChunkAddressText = nullptr;
    }
}

void AVoxelChunk::AddVoxelBillboards()
{
    if (VoxelBillboards.Num() > 0)
    {
        RemoveVoxelBillboards();
    }

    for (uint32 x = 0U; x < ChunkSize; ++x)
    {
        for (uint32 y = 0U; y < ChunkSize; ++y)
        {
            for (uint32 z = 0U; z < ChunkSize; ++z)
            {
                FVector VoxelPosition = FVector(x, y, z);

                // Draw billboards for empty voxels
                uint8 VoxelType = GetVoxel(x, y, z);
                EVoxelType eVoxelType = static_cast<EVoxelType>(VoxelType);

                if (eVoxelType == EVoxelType::AIR)
                {
                    FString strAddress = FString::Printf(TEXT("%d, %d, %d"), x, y, z);

                    UTextRenderComponent* VoxelBillboard = NewObject<UTextRenderComponent>(this);
                    VoxelBillboard->RegisterComponent();
                    VoxelBillboard->SetText(FText::FromString(strAddress));
                    VoxelBillboard->SetWorldLocation(GetActorLocation() - FVector(VoxelSize * ChunkSize / 2.F) + VoxelPosition * VoxelSize + FVector(VoxelSize / 2.F));
                    VoxelBillboard->SetWorldSize(10.F);
                    VoxelBillboard->SetTextRenderColor(FColor::Red);
                    VoxelBillboard->SetVisibility(true);
                    VoxelBillboard->SetHorizontalAlignment(EHTA_Center);
                    VoxelBillboard->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
                    VoxelBillboards.Add(VoxelBillboard);
                }
            }
        }
    }
}

void AVoxelChunk::RemoveVoxelBillboards()
{
    for (auto* VoxelBillboard : VoxelBillboards)
    {
        if (VoxelBillboard != nullptr)
        {
            VoxelBillboard->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            VoxelBillboard->DestroyComponent();
            VoxelBillboard = nullptr;
        }
    }

    VoxelBillboards.Empty();
}

float AVoxelChunk::GetOcclusionLvl() const
{
    return OcclusionLevel;
}

void AVoxelChunk::ApplyVoxelUpdates(const FChunkDataContainer& VoxelUpdateData)
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::ApplyVoxelUpdates called on uninitialized chunk"));
        return;
    }

    for (const auto& UpdatePair: VoxelUpdateData.VoxelStatesMap)
    {
        const FVoxelCoordinate& VoxelCoord = UpdatePair.Key;
        const FVoxelDefinition& VoxelDef = UpdatePair.Value;

        // Validate coordinates
        if (VoxelCoord.X < 0 || VoxelCoord.X >= 16 ||
            VoxelCoord.Y < 0 || VoxelCoord.Y >= 16 ||
            VoxelCoord.Z < 0 || VoxelCoord.Z >= 16)
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid voxel coordinate: %d, %d, %d"),
                   VoxelCoord.X, VoxelCoord.Y, VoxelCoord.Z);
            continue;
        }

        const int32 Index = VoxelCoord.X + VoxelCoord.Y * ChunkSize + VoxelCoord.Z * ChunkSize * ChunkSize;
        
        if (Voxels.IsValidIndex(Index))
        {
            UE_LOG(LogVoxelChunk, Log, TEXT("Chunk %lld, %lld, %lld Applying update for voxel %d, %d, %d"),
                   VoxelUpdateData.ChunkCoordinate.X, VoxelUpdateData.ChunkCoordinate.Y,
                   VoxelUpdateData.ChunkCoordinate.Z, VoxelCoord.X, VoxelCoord.Y, VoxelCoord.Z);
            Voxels[Index] = VoxelDef.VoxelType;
            VoxelStates[Index] = VoxelDef.VoxelState;
        }
        
    }

    RegenerateChunk();
    
}

void AVoxelChunk::RegenerateChunk()
{
    if (!bInitialized)
    {
        UE_LOG(LogVoxelChunk, Warning, TEXT("AVoxelChunk::RegenerateChunk called on uninitialized chunk"));
        return;
    }

    mesh->ClearMeshSection(0);
    mesh->ClearAllMeshSections();

    ClearAllVLOInstances();
    
    struct FMeshData
    {
        TArray<FVector> Vertices;
        TArray<FLinearColor> VertexColors;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FProcMeshTangent> Tangents;
        int32 VertexOffset = 0;
        UMaterialInstanceDynamic* AtlasMaterial;
    };

    TMap<int64, FMeshData> OverrideMeshData;
    FMeshData DefaultMeshData;

    TMap<uint8, TArray<FTransform>> VLOInstancesTransforms;

    int32 CulledFaces = 0;

    for (uint32 x = 0U; x < ChunkSize; ++x)
    {
        for (uint32 y = 0U; y < ChunkSize; ++y)
        {
            for (uint32 z = 0U; z < ChunkSize; ++z)
            {
                FVector VoxelPosition = FVector(x, y, z);

                // Determine the voxel type and corresponding color
                uint8 VoxelType = GetVoxel(x, y, z);

                // Get Voxel State
                FVoxelState& CurrentVoxelState = GetVoxelState(x, y, z);
                
                // Get Placeable Manager to spawn game objects
                TArray<AActor*> FoundActors;
                UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlaceableObjectManager::StaticClass(), FoundActors);

                APlaceableObjectManager* Manager = nullptr;
                if (FoundActors.Num() > 0)
                {
                    Manager = Cast<APlaceableObjectManager>(FoundActors[0]);
                }

                if (Manager)
                {
                    for (auto& ObjState : CurrentVoxelState.GameObjects)
                    {
                        UInstancedStaticMeshComponent* OutHISM = nullptr;
                        int32 OutInstanceIndex = INDEX_NONE;

                        FVector ObjWrldLocation = GetVoxelCenter(VoxelPosition) + ObjState.Location;

                        UE_LOG(LogVoxelChunk, Log, TEXT("Spawning object at location (%f, %f, %f)"), ObjWrldLocation.X, ObjWrldLocation.Y, ObjWrldLocation.Z);

                        Manager->SpawnById(
                            FName(*ObjState.ObjectID),
                            FTransform(ObjState.Rotation, ObjWrldLocation, ObjState.Scale),
                            this,
                            OutHISM, OutInstanceIndex
                        );
                    }
                }
                else
                {
                    UE_LOG(LogVoxelChunk, Error, TEXT("Placeable object manager not found! Unable to spawn game objects"));
                }

                if (VoxelType == static_cast<uint8>(EVoxelType::AIR))
	            {
	                continue;
	            }

                // In the VLO section of RegenerateChunk(), add detailed logging:
                if (CurrentVoxelState.IsVLO() && VLOMeshProvider)
                {
                    if (VLOMeshProvider->HasVLOMesh(VoxelType))
                    {
                        FVector LocalVoxelPosition;
                        LocalVoxelPosition.X = (VoxelPosition.X - ChunkSize / 2.0f + 0.5f) * VoxelSize;
                        LocalVoxelPosition.Y = (VoxelPosition.Y - ChunkSize / 2.0f + 0.5f) * VoxelSize;
                        LocalVoxelPosition.Z = (VoxelPosition.Z - ChunkSize / 2.0f) * VoxelSize;

                        FVector WorldPosition = GetActorLocation() + LocalVoxelPosition;
                         
                        FRotator VLORotation = CalculateVLORotation(CurrentVoxelState.FaceOneDirection, CurrentVoxelState.Rotation);
                        FVector AdjustedPosition = CalculateVLOPosition(WorldPosition, CurrentVoxelState.FaceOneDirection, CurrentVoxelState.Rotation);
                         
                        // Debug logging
                        UE_LOG(LogVoxelChunk, Warning, TEXT("VLO Debug - Chunk: (%lld,%lld,%lld), VoxelPos: (%d,%d,%d), LocalPos: (%.2f,%.2f,%.2f), ChunkWorldPos: (%.2f,%.2f,%.2f)"), 
                               X, Y, Z,
                               (int32)VoxelPosition.X, (int32)VoxelPosition.Y, (int32)VoxelPosition.Z,
                               LocalVoxelPosition.X, LocalVoxelPosition.Y, LocalVoxelPosition.Z,
                               GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
                         
                        FTransform VLOTransform = FTransform(VLORotation, AdjustedPosition, FVector::OneVector);
                        VLOInstancesTransforms.FindOrAdd(VoxelType).Add(VLOTransform);
                        continue;
                    }
                    else
                    {
                        CurrentVoxelState.bIsVLO = false;
                    }
                }


                // Cull hidden faces // Bottom Front Top Right Back Left
                // Cull hidden faces // Bottom Front Top Right Back Left
                bool VisibleFaces[6] = {
                    (z == 0U)            || IsVoxelTransparentForCulling(x, y, z-1),
                    (x == 0U)            || IsVoxelTransparentForCulling(x-1, y, z),
                    (z == ChunkSize - 1) || IsVoxelTransparentForCulling(x, y, z+1),
                    (y == ChunkSize - 1) || IsVoxelTransparentForCulling(x, y+1, z),
                    (x == ChunkSize - 1) || IsVoxelTransparentForCulling(x+1, y, z),
                    (y == 0U)            || IsVoxelTransparentForCulling(x, y-1, z)
                };


                UEnum* EnumPtr = StaticEnum<EVoxelType>();

                float VoxelTypeIndex = static_cast<float>(VoxelType - 1) / static_cast<float>(EnumPtr->GetMaxEnumValue() - 1);

                FVector HalfSize = FVector(ChunkSize * VoxelSize / 2);

                if (CurrentVoxelState.AtlasOverride > 0)
                {
                    FMeshData& MeshData = OverrideMeshData.FindOrAdd(CurrentVoxelState.AtlasOverride);
                    GenerateCubeMesh(VoxelPosition, HalfSize, VoxelSize, VisibleFaces, MeshData.Vertices, MeshData.Triangles,
                                     MeshData.Normals, MeshData.UVs, MeshData.VertexOffset, CulledFaces,
                                     MeshData.VertexColors, VoxelTypeIndex, CurrentVoxelState, MeshData.Tangents);
                }
                else
                {
                    GenerateCubeMesh(VoxelPosition, HalfSize, VoxelSize, VisibleFaces, DefaultMeshData.Vertices, DefaultMeshData.Triangles,
                                     DefaultMeshData.Normals, DefaultMeshData.UVs, DefaultMeshData.VertexOffset, CulledFaces,
                                     DefaultMeshData.VertexColors, VoxelTypeIndex, CurrentVoxelState, DefaultMeshData.Tangents);
                }
            }
        }
    }

    DefaultMeshData.AtlasMaterial = DynamicMaterialInstance;
    mesh->CreateMeshSection_LinearColor(0, DefaultMeshData.Vertices, DefaultMeshData.Triangles, DefaultMeshData.Normals, DefaultMeshData.UVs, DefaultMeshData.VertexColors, DefaultMeshData.Tangents, true);
    mesh->SetMaterial(0, DefaultMeshData.AtlasMaterial);
    
    int64 SectionIndex = 1;
    for (auto& Pair : OverrideMeshData)
    {
        const int64 AtlasOverride = Pair.Key;
        FMeshData& MeshData = Pair.Value;
        mesh->CreateMeshSection_LinearColor(SectionIndex, MeshData.Vertices, MeshData.Triangles, MeshData.Normals, MeshData.UVs, MeshData.VertexColors, MeshData.Tangents, true);
        if (AtlasManager->DoesAtlasExist(AtlasOverride))
        {
            UTexture* AtlasOverrideTexture = AtlasManager->GetAtlas(AtlasOverride);
            UMaterialInstanceDynamic* NewMat = CreateNewMaterialInstance(AtlasOverrideTexture);
            MeshData.AtlasMaterial = NewMat;
        }
        else
        {
            AtlasManager->RequestAtlas(AtlasOverride);
        }
        
        mesh->SetMaterial(SectionIndex, MeshData.AtlasMaterial);
        SectionIndex++;
    }

    // Generate VLO instances
    for (const auto& VLOPair : VLOInstancesTransforms)
    {
        uint8 VoxelType = VLOPair.Key;
        const TArray<FTransform>& VLOTransforms = VLOPair.Value;
    
        UE_LOG(LogVoxelChunk, Log, TEXT("Processing VLO instances for voxel type %d: %d instances"), VoxelType, VLOTransforms.Num());
    
        if (VLOMeshProvider)
        {
            UStaticMesh* VLOMesh = VLOMeshProvider->GetVLOMesh(VoxelType);
            if (VLOMesh)
            {
                UE_LOG(LogVoxelChunk, Log, TEXT("Found VLO mesh for voxel type %d: %s"), VoxelType, *VLOMesh->GetName());
            
                if (UInstancedStaticMeshComponent* VLOComponent = GetOrCreateVLOComponent(VoxelType, VLOMesh))
                {
                    UE_LOG(LogVoxelChunk, Log, TEXT("Adding %d instances to VLO component for voxel type %d"), VLOTransforms.Num(), VoxelType);
                    VLOComponent->AddInstances(VLOTransforms, false, true);
                    UE_LOG(LogVoxelChunk, Log, TEXT("Successfully added VLO instances. Total instances in component: %d"), VLOComponent->GetInstanceCount());
                }
                else
                {
                    UE_LOG(LogVoxelChunk, Error, TEXT("Failed to get or create VLO component for voxel type %d"), VoxelType);
                }
            }
            else
            {
                UE_LOG(LogVoxelChunk, Warning, TEXT("No VLO mesh found for voxel type %d"), VoxelType);
            }
        }
        else
        {
            UE_LOG(LogVoxelChunk, Error, TEXT("VLOMeshProvider is null - cannot create VLO instances"));
        }
    }
	//UE_LOG(LogVoxelChunk, Log, TEXT("Generated mesh: %d (%d)"), Vertices.Num(), CulledFaces);
}

void AVoxelChunk::GenerateCubeMesh(FVector Position, FVector HalfSize, float VoxelSize, const bool VisibleFaces[6], TArray<FVector>& Vertices, TArray<int32>& Triangles,
                                    TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexOffset, int32& culledFaces, TArray<FLinearColor>& VertexColors, float VoxelTypeIndex, FVoxelState
                                    & State, TArray<FProcMeshTangent>& Tangents)
  {
      FVector PositionOffset = Position * VoxelSize;
  
      // Define the cube vertices
      TArray<FVector> LocalVertices = {
          PositionOffset + FVector(0,         0,         0        ) - HalfSize, // 0
          PositionOffset + FVector(0,         VoxelSize, 0        ) - HalfSize, // 1
          PositionOffset + FVector(VoxelSize, VoxelSize, 0        ) - HalfSize, // 2
          PositionOffset + FVector(VoxelSize, 0,         0        ) - HalfSize, // 3
          PositionOffset + FVector(VoxelSize, 0,         VoxelSize) - HalfSize, // 4
          PositionOffset + FVector(VoxelSize, VoxelSize, VoxelSize) - HalfSize, // 5
          PositionOffset + FVector(0,         VoxelSize, VoxelSize) - HalfSize, // 6
          PositionOffset + FVector(0,         0,         VoxelSize) - HalfSize  // 7
      };
  
      // The order of faces in the VisibleFaces array is:
      // [0] Bottom (-Z), [1] Front (-X), [2] Top (+Z), [3] Right (+Y), [4] Back (+X), [5] Left (-Y)
      
      // Face vertices indices matching your VisibleFaces order
      const int32 FaceVerticesIndices[6][4] = {
          { 0, 1, 2, 3 },   // [0] Bottom (-Z) face
          { 0, 7, 6, 1 },   // [1] Front (-X) face
          { 4, 5, 6, 7 },   // [2] Top (+Z) face
          { 1, 6, 5, 2 },   // [3] Right (+Y) face
          { 3, 2, 5, 4 },   // [4] Back (+X) face
          { 0, 3, 4, 7 }    // [5] Left (-Y) face
      };
      
      // Face normals matching your VisibleFaces order
      const FVector FaceNormals[6] = {
          FVector(0, 0, -1),  // [0] Bottom (-Z)
          FVector(-1, 0, 0),  // [1] Front (-X)
          FVector(0, 0, 1),   // [2] Top (+Z)
          FVector(0, 1, 0),   // [3] Right (+Y)
          FVector(1, 0, 0),   // [4] Back (+X)
          FVector(0, -1, 0)   // [5] Left (-Y)
      };
  
      static const FVector2D FaceUVs[4] = {
          FVector2D(0, 1), FVector2D(1, 1), FVector2D(1, 0), FVector2D(0, 0)
      };
  
    // Generate tangents for each face
      const FProcMeshTangent FaceTangents[6] =
      {
            FProcMeshTangent(-1, 0, 0),       // Bottom (-Z)
            FProcMeshTangent(0, 1, 0),       // Front (-X)
            FProcMeshTangent(1, 0, 0),       // Top (+Z): Known to work
            FProcMeshTangent(1, 0, 0),       // Right (+Y)
            FProcMeshTangent(0, -1, 0),       // Back (+X)
            FProcMeshTangent(-1, 0, 0)        // Left (-Y)
      };


  
      // Reset culled faces counter
      culledFaces = 0;
  
      // Loop through each face and generate the mesh
      for (int32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
      {
          if (VisibleFaces[FaceIndex])
          {
              FVector2D UV0 = FaceUVs[0];
              FVector2D UV1 = FaceUVs[1];
              FVector2D UV2 = FaceUVs[2];
              FVector2D UV3 = FaceUVs[3];
  
              int32 RotationAngle;
              float AtlasFace;
              
              DetermineVoxelFaces(State.FaceOneDirection, State.Rotation, FaceIndex, AtlasFace);
              DetermineVoxelFaceRotations(State.FaceOneDirection, State.Rotation, FaceIndex, RotationAngle);
              RotateUVs(RotationAngle, UV0, UV1, UV2, UV3);
              
              // Calculate the indices for this face
              int32 v0 = FaceVerticesIndices[FaceIndex][0];
              int32 v1 = FaceVerticesIndices[FaceIndex][1];
              int32 v2 = FaceVerticesIndices[FaceIndex][2];
              int32 v3 = FaceVerticesIndices[FaceIndex][3];
              
              // Add vertices for the face
              Vertices.Add(LocalVertices[v0]);
              Vertices.Add(LocalVertices[v1]);
              Vertices.Add(LocalVertices[v2]);
              Vertices.Add(LocalVertices[v3]);
              
              // Add the same normal for all vertices of this face
              FVector NormalForFace = FaceNormals[FaceIndex];
              for (int32 i = 0; i < 4; ++i) {
                  Normals.Add(NormalForFace);
                  UVs.Add(i == 0 ? UV0 : (i == 1 ? UV1 : (i == 2 ? UV2 : UV3)));
                  FLinearColor VertexColor = FLinearColor(VoxelTypeIndex, AtlasFace, 0.0f, 1.0f);
                  VertexColors.Add(VertexColor);
                  Tangents.Add(FaceTangents[FaceIndex]);
              }
  
              // Flip the winding order to render faces correctly outwardly
              // First triangle
              Triangles.Add(VertexOffset + 0);
              Triangles.Add(VertexOffset + 2);
              Triangles.Add(VertexOffset + 1);
              
              // Second triangle
              Triangles.Add(VertexOffset + 0);
              Triangles.Add(VertexOffset + 3);
              Triangles.Add(VertexOffset + 2);
  
              // Increment the vertex offset by 4 as we just added 4 vertices for this face
              VertexOffset += 4;
          }
          else
          {
              culledFaces++;
          }
      }
  }

void AVoxelChunk::RotateUVs(const int32 Rotation, FVector2D& UV0, FVector2D& UV1, FVector2D& UV2, FVector2D& UV3)
{
    switch (Rotation)
    {
    case 90:
        // Rotate UVs 90 degrees clockwise
        UV0 = FVector2D(1.0f - UV0.Y, UV0.X); // Swap and flip U
        UV1 = FVector2D(1.0f - UV1.Y, UV1.X); // Swap and flip U
        UV2 = FVector2D(1.0f - UV2.Y, UV2.X); // Swap and flip U
        UV3 = FVector2D(1.0f - UV3.Y, UV3.X); // Swap and flip U
        break;

    case 180:
        // Rotate UVs 180 degrees
        UV0 = FVector2D(1.0f - UV0.X, 1.0f - UV0.Y); // Flip both U and V
        UV1 = FVector2D(1.0f - UV1.X, 1.0f - UV1.Y); // Flip both U and V
        UV2 = FVector2D(1.0f - UV2.X, 1.0f - UV2.Y); // Flip both U and V
        UV3 = FVector2D(1.0f - UV3.X, 1.0f - UV3.Y); // Flip both U and V
        break;

    case 270:
        // Rotate UVs 270 degrees clockwise
        UV0 = FVector2D(UV0.Y, 1.0f - UV0.X); // Swap and flip V
        UV1 = FVector2D(UV1.Y, 1.0f - UV1.X); // Swap and flip V
        UV2 = FVector2D(UV2.Y, 1.0f - UV2.X); // Swap and flip V
        UV3 = FVector2D(UV3.Y, 1.0f - UV3.X); // Swap and flip V
        break;

    default:
        // No rotation
        break;
    }
}

void AVoxelChunk::DetermineVoxelFaceRotations(const uint8& FaceOneDirection, const uint8& Rotation, const int32& FaceIndex, int32& RotationAngle)
{
    // 0-5 = Bottom, Front, Top, Right, Back, Left
    constexpr int RotationLookup[4][6][6] = {
        // Rotation = 0
        {
            {0, 270, 0, 270, 0, 0},      // FaceOneDirection = 0
            {180, 90, 0, 180, 0, 90},      // FaceOneDirection = 1
            {270, 0, 270, 0, 270, 270},    // FaceOneDirection = 2
            {0, 90, 180, 0, 0, 270},    // FaceOneDirection = 3
            {90, 180, 90, 0, 90, 90},          // FaceOneDirection = 4
            {0, 90, 0, 90, 180, 180},    // FaceOneDirection = 5
        },
        // Rotation = 1
        {
            {90, 270, 90, 270, 0, 0},    // FaceOneDirection = 0
            {270, 0, 270, 90, 270, 0},     // FaceOneDirection = 1
            {0, 180, 180, 0, 90, 270},     // FaceOneDirection = 2
            {90, 180, 90, 90, 90, 0},     // FaceOneDirection = 3
            {180, 0, 0, 180, 90, 90},     // FaceOneDirection = 4
            {90, 90, 270, 90, 180, 180},   // FaceOneDirection = 5
        },
        // Rotation = 2
        {
            {180, 270, 180, 270, 0, 0},      // FaceOneDirection = 0
            {0, 270, 180, 0, 0, 270},     // FaceOneDirection = 1
            {90, 180, 90, 180, 90, 90},    // FaceOneDirection = 2
            {180, 90, 0, 180, 180, 90},      // FaceOneDirection = 3
            {270, 0, 270, 0, 270, 90},     // FaceOneDirection = 4
            {180, 90, 180, 90, 180, 180},   // FaceOneDirection = 5
        },
        // Rotation = 3
        {
            {270, 270, 270, 270, 0, 0},         // FaceOneDirection = 0
            {90, 180, 90, 270, 90, 0},        // FaceOneDirection = 1
            {180, 0, 0, 0, 90, 90},     // FaceOneDirection = 2
            {270, 0, 270, 90, 270, 180},     // FaceOneDirection = 3
            {0, 0, 180, 0, 90, 270},   // FaceOneDirection = 4
            {270, 90, 90, 90, 180, 180},    // FaceOneDirection = 5
        },
    };
    
    if (Rotation < 4 && FaceOneDirection < 6 && FaceIndex < 6) {
        RotationAngle = RotationLookup[Rotation][FaceOneDirection][FaceIndex];
    }
    
}

void AVoxelChunk::DetermineVoxelFaces(const uint8& FaceOneDirection, const uint8& Rotation,
                                      const int32& FaceIndex, float& AtlasFace)
{

     // Atlas UV coordinates (as proportion):
    // 0.0 = Top (Diamond)
    // 0.2 = Front (Up Arrow)
    // 0.4 = Right (Right Arrow)
    // 0.6 = Back (Left Arrow)
    // 0.8 = Left (Down Arrow)
    // 1.0 = Bottom (Star)

    constexpr float DIAMOND = 0.0f;
    constexpr float UP_ARROW = 0.2f;
    constexpr float RIGHT_ARROW = 0.4f;
    constexpr float LEFT_ARROW = 0.6f;
    constexpr float DOWN_ARROW = 0.8f;
    constexpr float STAR = 1.0f;



    // Order of faces: { Bottom, Front, Top, Right, Back, Left }
    float faces[6] = { STAR, UP_ARROW, DIAMOND, RIGHT_ARROW, LEFT_ARROW, DOWN_ARROW };

    // Handle initial mapping based on FaceOneDirection
    switch (FaceOneDirection) {
        case 0:
            // Original orientation (FaceOneDirection = 0)
            break;  // No change needed here
        case 1:
            // Diamond moves to front (FaceOneDirection = 1)
            faces[0] = UP_ARROW;
            faces[1] = DIAMOND;
            faces[2] = LEFT_ARROW;  // Top face
            faces[3] = RIGHT_ARROW;
            faces[4] = STAR;
            faces[5] = DOWN_ARROW;
            break;
        case 2:
            // Diamond moves to right (FaceOneDirection = 2)
            faces[0] = RIGHT_ARROW;
            faces[1] = UP_ARROW;  // Front face
            faces[2] = DOWN_ARROW; // Top face
            faces[3] = DIAMOND;  // Right face
            faces[4] = LEFT_ARROW; // Back face
            faces[5] = STAR;      // Left face
            break;
        case 3:
            // Diamond moves to back (FaceOneDirection = 3)
            faces[0] = LEFT_ARROW;
            faces[1] = STAR;  // Front face
            faces[2] = UP_ARROW;    // Top face
            faces[3] = RIGHT_ARROW; // Right face
            faces[4] = DIAMOND;     // Back face
            faces[5] = DOWN_ARROW;  // Left face
            break;
        case 4:
            // Diamond moves to left (FaceOneDirection = 4)
            faces[0] = DOWN_ARROW;
            faces[1] = UP_ARROW;  // Front face
            faces[2] = RIGHT_ARROW; // Top face
            faces[3] = STAR;      // Right face
            faces[4] = LEFT_ARROW; // Back face
            faces[5] = DIAMOND;   // Left face
            break;
        case 5:
            // Diamond moves to bottom (FaceOneDirection = 5)
            faces[0] = DIAMOND;
            faces[1] = LEFT_ARROW; // Front face
            faces[2] = STAR;       // Top face
            faces[3] = RIGHT_ARROW;
            faces[4] = UP_ARROW;
            faces[5] = DOWN_ARROW;
            break;
    }

    // Handle rotation based on Rotation value (0 to 3)
    if (Rotation == 1) {
        // After 90° rotation (counter-clockwise), update the side faces
        float tempFront = faces[1];
        faces[1] = faces[5];  // Front becomes Left
        faces[5] = faces[4];  // Left becomes Back
        faces[4] = faces[3];  // Back becomes Right
        faces[3] = tempFront; // Right becomes Front
    }
    else if (Rotation == 2) {
        // After 180° rotation, swap Front and Back, Left and Right
        float tempFront = faces[1];
        faces[1] = faces[4];  // Front becomes Back
        faces[4] = tempFront;  // Back becomes Front

        float tempLeft = faces[5];
        faces[5] = faces[3];  // Left becomes Right
        faces[3] = tempLeft;   // Right becomes Left
    }
    else if (Rotation == 3) {
        // After 270° rotation (counter-clockwise), update the side faces
        float tempFront = faces[1];
        faces[1] = faces[3];  // Front becomes Right
        faces[3] = faces[4];  // Right becomes Back
        faces[4] = faces[5];  // Back becomes Left
        faces[5] = tempFront; // Left becomes Front
    }

    // Based on FaceIndex, assign the correct face value
    switch (FaceIndex) {
        case 0:  // Bottom
            AtlasFace = faces[0];  // Bottom stays the same in all rotations
            break;
        case 1:  // Front
            AtlasFace = faces[1];  // Front face value after rotation
            break;
        case 2:  // Top
            AtlasFace = faces[2];  // Top stays the same in all rotations
            break;
        case 3:  // Right
            AtlasFace = faces[3];  // Right face value after rotation
            break;
        case 4:  // Back
            AtlasFace = faces[4];  // Back face value after rotation
            break;
        case 5:  // Left
            AtlasFace = faces[5];  // Left face value after rotation
            break;
        default:
            // Handle unexpected FaceIndex if necessary
            AtlasFace = DIAMOND;  // Default value
            break;
    }
}

UMaterialInstanceDynamic* AVoxelChunk::CreateNewMaterialInstance(UTexture* InTexture)
{
    UMaterialInstanceDynamic* NewAtlasMaterial = UMaterialInstanceDynamic::Create(MaterialInstance, this);
    NewAtlasMaterial->SetTextureParameterValue("TextureAtlas", InTexture);
    return NewAtlasMaterial;
}


void AVoxelChunk::RotateVoxel(const int x, const int y, const int z)
{
    FVoxelState& CurrentVoxelState = GetVoxelState(x, y, z);

    if (CurrentVoxelState.Rotation == 3)
    {
        CurrentVoxelState.Rotation = 0;
    }
    else
    {
        CurrentVoxelState.Rotation = CurrentVoxelState.Rotation + 1;
    }

    RegenerateChunk();
}

void AVoxelChunk::CycleFaceOneDirection(const int x, const int y, const int z)
{
    FVoxelState& CurrentState = GetVoxelState(x, y, z);

    if (CurrentState.FaceOneDirection == 5)
    {
        CurrentState.FaceOneDirection = 0;
    }
    else
    {
        CurrentState.FaceOneDirection = CurrentState.FaceOneDirection + 1;
    }
    
    RegenerateChunk();
}

void AVoxelChunk::ChangeAtlasType(const int x, const int y, const int z, const int64 atlasType)
{
    FVoxelState& CurrentState = GetVoxelState(x, y, z);
    CurrentState.AtlasOverride = atlasType;
    RegenerateChunk();
}

void AVoxelChunk::UpdateVoxelState(const int x, const int y, const int z, const FVoxelState& State)
{
    FVoxelState& VoxelState = GetVoxelState(x, y, z);
    VoxelState.Rotation = State.Rotation;
    VoxelState.FaceOneDirection = State.FaceOneDirection;
    VoxelState.AtlasOverride = State.AtlasOverride;
    VoxelState.bIsVLO = State.bIsVLO;
}

FVoxelState& AVoxelChunk::GetVoxelState(const int x, const int y, const int z)
{
    const uint32 Index = x + y * ChunkSize + z * ChunkSize * ChunkSize;
    return VoxelStates[Index];
}

bool AVoxelChunk::IsNonStandardVoxel(const int x, const int y, const int z)
{
    FVoxelState& VoxelState = GetVoxelState(x, y ,z);

    if (VoxelState.Rotation != 0 || VoxelState.FaceOneDirection != 0 || VoxelState.AtlasOverride != 0)
    {
        return true;
    }

    return false;
}

void AVoxelChunk::SetAtlasManagerReference(AAtlasManager* InAtlasManger)
{
    AtlasManager = InAtlasManger;
    AtlasManager->OnAtlasLoaded.AddDynamic(this, &AVoxelChunk::HandleAtlasOverrides);
}

void AVoxelChunk::HandleAtlasOverrides(UTexture* inAtlas)
{
    RegenerateChunk();
}

bool AVoxelChunk::IsVoxelTransparentForCulling(const uint32 Vx, const uint32 Vy, const uint32 Vz)
{
    uint8 VoxelType = GetVoxel(Vx, Vy, Vz);

    if (VoxelType == static_cast<uint8>(EVoxelType::AIR))
    {
        return true;
    }

    const FVoxelState& VoxelState = GetVoxelState(Vx, Vy, Vz);
    return VoxelState.IsVLO();
}

// VLOs

FRotator AVoxelChunk::CalculateVLORotation(uint8 FaceOneDirection, uint8 Rotation) const
{
    // Target direction for the mesh's local Z axis in world space
    FVector TargetZ;
    
    switch (FaceOneDirection)
    {
    case 0: TargetZ = FVector::UpVector;            break;  // Z+
    case 1: TargetZ = FVector::BackwardVector;      break;  // -X
    case 2: TargetZ = FVector::RightVector;         break;  // +Y
    case 3: TargetZ = FVector::ForwardVector;       break;  // +X
    case 4: TargetZ = FVector::LeftVector;          break;  // -Y
    case 5: TargetZ = FVector::DownVector;          break;  // -Z
    default:
        UE_LOG(LogVoxelChunk, Warning, TEXT("Invalid FaceOneDirection: %d"), FaceOneDirection);
        TargetZ = FVector::UpVector;
        break;
    }

    // The mesh starts with Z+ pointing "up" in the local space
    const FVector SourceZ = FVector::UpVector;

    // Determine rotation needed to align SourceZ with TargetZ
    const FQuat AlignZQuat = FQuat::FindBetweenNormals(SourceZ, TargetZ);

    // Now apply a Yaw rotation in world Z (after the mesh has been reoriented)
    const float Degrees = (Rotation % 4) * 90.0f;
    const FQuat WorldYawQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(-Degrees));

    // Apply yaw AFTER aligning the Z axis (i.e., rotate the aligned mesh in world Z)
    const FQuat FinalQuat = WorldYawQuat * AlignZQuat;

    return FinalQuat.Rotator();
}

FVector AVoxelChunk::CalculateVLOPosition(const FVector& BasePosition, uint8 FaceOneDirection, uint8 Rotation) const
{
    // Start at base position (center of voxel)
    const FVector CenteredPosition = BasePosition + FVector(0, 0, VoxelSize * 0.5f);

    // Offset from pivot (assumed to be bottom center) to center of mesh
    const FVector LocalOffset = FVector(0, 0, -VoxelSize * 0.5f); // move up half-voxel to center

    // Calculate full rotation so we can rotate the offset
    const FRotator RotationFinal = CalculateVLORotation(FaceOneDirection, Rotation);
    const FVector RotatedOffset = RotationFinal.RotateVector(LocalOffset);

    // Apply rotated offset
    return CenteredPosition + RotatedOffset;
}

void AVoxelChunk::SetVLOMeshProvider(AVLOMeshProvider* InVLOMeshProvider)
{
    VLOMeshProvider = InVLOMeshProvider;
    UE_LOG(LogVoxelChunk, Log, TEXT("VLO Mesh Provider set for chunk (%lld, %lld, %lld)"), X, Y, Z);
}

AVLOMeshProvider* AVoxelChunk::GetVLOMeshProvider() const
{
    return VLOMeshProvider;
}

bool AVoxelChunk::IsVLOAtPosition(const int x, const int y, const int z) const
{
    const FVoxelState& State = const_cast<AVoxelChunk*>(this)->GetVoxelState(x, y, z);
    return State.IsVLO();
}

FVector AVoxelChunk::GetVoxelCenter(const FVector VoxelPosition) const
{
    FVector LocalVoxelPosition = VoxelPosition * VoxelSize;
    LocalVoxelPosition += FVector(VoxelSize / 2);
    LocalVoxelPosition -= FVector(ChunkSize * VoxelSize / 2);

    FVector WorldPosition = GetActorLocation() + LocalVoxelPosition;
    return WorldPosition;
}

void AVoxelChunk::ClearAllVLOInstances()
{
    for (auto& Pair : VLOInstancedMeshes)
    {
        if (UInstancedStaticMeshComponent* Component = Pair.Value)
        {
            Component->ClearInstances();
        }
    }

    for (auto& Pair : ObjectInstancedMeshes)
    {
        if (TObjectPtr<UInstancedStaticMeshComponent> Component = Pair.Value)
        {
            Component->ClearInstances();
        }
    }
}

UInstancedStaticMeshComponent* AVoxelChunk::GetOrCreateVLOComponent(uint8 VoxelType, UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        UE_LOG(LogVoxelChunk, Error, TEXT("GetOrCreateVLOComponent called with null mesh for voxel type %d"), VoxelType);
        return nullptr;
    }

    if (UInstancedStaticMeshComponent** FoundComponent = VLOInstancedMeshes.Find(VoxelType))
    {
        UE_LOG(LogVoxelChunk, Verbose, TEXT("Using existing VLO component for voxel type %d"), VoxelType);
        return *FoundComponent;
    }

    UE_LOG(LogVoxelChunk, Log, TEXT("Creating new VLO component for voxel type %d with mesh %s"), VoxelType, *Mesh->GetName());

    // Create new component
    UInstancedStaticMeshComponent* NewComponent = NewObject<UInstancedStaticMeshComponent>(
        this, 
        UInstancedStaticMeshComponent::StaticClass(),
        *FString::Printf(TEXT("VLOInstancedMesh_%d"), VoxelType)
    );

    
    if (!NewComponent)
    {
        UE_LOG(LogVoxelChunk, Error, TEXT("Failed to create VLO component for voxel type %d"), VoxelType);
        return nullptr;
    }
    
    NewComponent->SetStaticMesh(Mesh);
    
    // Debug: Check mesh validity
    if (NewComponent->GetStaticMesh())
    {
        UE_LOG(LogVoxelChunk, Log, TEXT("VLO component mesh set successfully: %s"), *NewComponent->GetStaticMesh()->GetName());
    }
    else
    {
        UE_LOG(LogVoxelChunk, Error, TEXT("Failed to set mesh on VLO component for voxel type %d"), VoxelType);
    }
    
    // Debug: Visibility and rendering settings
    NewComponent->SetVisibility(true);
    NewComponent->SetHiddenInGame(false);
    NewComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    NewComponent->SetCastShadow(true);
    
    // Debug: Component registration
    NewComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
    NewComponent->RegisterComponent();
    
    // Debug: Check if component is properly registered
    if (NewComponent->IsRegistered())
    {
        UE_LOG(LogVoxelChunk, Log, TEXT("VLO component registered successfully for voxel type %d"), VoxelType);
    }
    else
    {
        UE_LOG(LogVoxelChunk, Error, TEXT("VLO component failed to register for voxel type %d"), VoxelType);
    }
    
    VLOInstancedMeshes.Add(VoxelType, NewComponent);
    
    UE_LOG(LogVoxelChunk, Log, TEXT("Successfully created and registered VLO component for voxel type %d"), VoxelType);
    return NewComponent;
}
