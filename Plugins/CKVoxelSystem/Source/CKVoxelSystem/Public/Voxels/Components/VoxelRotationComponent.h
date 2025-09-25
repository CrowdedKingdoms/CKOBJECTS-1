#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
// #include "Network/Services/GameData/VoxelServiceSubsystem.h"
// #include "Voxels/Core/VoxelWorldSubsystem.h"
#include "VoxelRotationComponent.generated.h"

class UVoxelManager;    
class UVoxelServiceSubsystem;
class UVoxelWorldSubsystem;

UCLASS(ClassGroup=(VoxelFeature), Blueprintable, meta=(BlueprintSpawnableComponent))
class UVoxelRotationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVoxelRotationComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelRotation")
    UVoxelManager* VoxelManager = nullptr;
    
protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelRotation")
    UVoxelServiceSubsystem* VoxelServiceSubsystem = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VoxelRotation")
    UVoxelWorldSubsystem* VoxelWorldSubsystem = nullptr;

private:
    TWeakObjectPtr<AActor> MyOwner;
};
