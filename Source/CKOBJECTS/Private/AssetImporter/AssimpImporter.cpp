#include "AssetImporter/AssimpImporter.h"
#include "AssetImporter/ImportedModelActor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include "Assimp/postprocess.h"

DEFINE_LOG_CATEGORY(LogAssimp);

AActor* UAssimpImporter::ImportModelToWorld(
    const UObject*      WorldContextObject,
    const FString&      FilePath,
    FVector             Location,
    UMaterialInterface* BaseMaterial
)
{
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogAssimp, Error, TEXT("File does not exist: %s"), *FilePath);
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World)
    {
        UE_LOG(LogAssimp, Error, TEXT("World context invalid."));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    AImportedModelActor* ImportedActor = World->SpawnActor<AImportedModelActor>(Location, FRotator::ZeroRotator, SpawnParams);

    if (!ImportedActor)
    {
        UE_LOG(LogAssimp, Error, TEXT("Failed to spawn ImportedModelActor."));
        return nullptr;
    }

    if (BaseMaterial)
    {
        ImportedActor->BaseMaterial = BaseMaterial;
    }
    else
    {
        UE_LOG(LogAssimp, Warning, TEXT("Failed to set base material."));
    }

    ImportedActor->LoadModelFromFile(FilePath);

    UE_LOG(LogAssimp, Log, TEXT("Imported model: %s at location %s"), *FilePath, *Location.ToString());

    return ImportedActor;
}

bool UAssimpImporter::ValidateModelFile(const FString& FilePath)
{
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogAssimp, Error, TEXT("File does not exist: %s"), *FilePath);
        return false;
    }

    Assimp::Importer Importer;
    const aiScene* Scene = Importer.ReadFile(
        TCHAR_TO_UTF8(*FilePath),
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
    );

    if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
    {
        UE_LOG(LogAssimp, Error, TEXT("Assimp failed to load model: %s"), *FString(Importer.GetErrorString()));
        return false;
    }

    UE_LOG(LogAssimp, Log, TEXT("Model is valid: %s"), *FilePath);
    return true;
}
