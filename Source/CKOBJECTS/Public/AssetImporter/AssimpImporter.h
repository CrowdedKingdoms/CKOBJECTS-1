#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssimpImporter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAssimp, Log, All);

UCLASS()
class   UAssimpImporter : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:    
    // Spawns a model into the world
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Assimp Importer", meta = (WorldContext = "WorldContextObject"))
    static AActor* ImportModelToWorld(
        const UObject*      WorldContextObject, 
        const FString&      FilePath, 
        FVector             Location,
        UMaterialInterface* BaseMaterial = nullptr
    );

    // Validates that a model file exists and is loadable
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Assimp Importer")
    static bool ValidateModelFile(const FString& FilePath);
};
