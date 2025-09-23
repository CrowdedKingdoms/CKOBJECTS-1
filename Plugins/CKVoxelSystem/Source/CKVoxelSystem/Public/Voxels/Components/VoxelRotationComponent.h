#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
// #include "Network/Services/GameData/VoxelServiceSubsystem.h"
// #include "Voxels/Core/VoxelWorldSubsystem.h"
#include "VoxelRotationComponent.generated.h"

class AVoxelChunk;
class UVoxelServiceSubsystem;
class UVoxelWorldSubsystem;

UCLASS(ClassGroup=(VoxelFeature), meta=(BlueprintSpawnableComponent))
class UVoxelRotationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVoxelRotationComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelRotation")
    UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelRotation")
    UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;

    
    
protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<AActor> Owner;
};
