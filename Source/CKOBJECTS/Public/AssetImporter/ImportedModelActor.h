#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ImportedModelActor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAssimpModel, Log, All);

UCLASS()
class   AImportedModelActor : public AActor
{
    GENERATED_BODY()

public:
    AImportedModelActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProceduralMeshComponent* ProcMesh;

    // Scale factor applied to vertices (modifiable in BP, triggers reimport)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
    float ScaleFactor = 50.f;

    // Rotation correction applied to vertices (modifiable in BP, triggers reimport)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
    FRotator RotationCorrection = FRotator(90.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
    TSet<int32> MeshSectionsToSkip = { };

    // Base material with "BaseTexture" parameter to apply textures
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Import Settings")
    UMaterialInterface* BaseMaterial;

    // Path of currently loaded model file
    UPROPERTY(BlueprintReadOnly, Category = "Import Settings")
    FString LoadedModelPath;

    // Load model from disk (OBJ + MTL + textures)
    UFUNCTION(BlueprintCallable, Category = "Import")
    void LoadModelFromFile(const FString& FilePath);

    // Reimport currently loaded model with current settings
    UFUNCTION(BlueprintCallable, Category = "Import")
    void Reimport();

    // Setter functions exposed to BP for live update
    UFUNCTION(BlueprintCallable, Category = "Import Settings")
    void SetScaleFactor(float NewScale);

    UFUNCTION(BlueprintCallable, Category = "Import Settings")
    void SetRotationCorrection(FRotator NewRotation);

    // Setter for MeshSectionsToSkip that triggers reimport
    UFUNCTION(BlueprintCallable, Category = "Import Settings")
    void SetMeshSectionsToSkip(const TSet<int32>& NewMeshSectionsToSkip);

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void LoadAssimpScene(const struct aiScene* Scene);

    // Loads a JPG texture from disk into UTexture2D
    UTexture2D* LoadTextureFromFile(const FString& FilePath);

    // Helper: Get absolute directory from LoadedModelPath
    FString GetModelDirectory() const;
};