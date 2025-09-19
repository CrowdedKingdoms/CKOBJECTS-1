#include "AssetImporter/ImportedModelActor.h"

#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include "Assimp/postprocess.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogAssimpModel);

AImportedModelActor::AImportedModelActor()
{
    ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
    RootComponent = ProcMesh;

    ProcMesh->bUseAsyncCooking = true;
}

void AImportedModelActor::LoadModelFromFile(const FString& FilePath)
{
    LoadedModelPath = FilePath;

    Assimp::Importer Importer;

    const aiScene* Scene = Importer.ReadFile(
        TCHAR_TO_UTF8(*FilePath),
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_ConvertToLeftHanded
    );

    if (!Scene || !Scene->HasMeshes())
    {
        UE_LOG(LogAssimpModel, Error, TEXT("Failed to load model from file: %s"), *FilePath);
        return;
    }

    LoadAssimpScene(Scene);
}

void AImportedModelActor::Reimport()
{
    if (LoadedModelPath.IsEmpty())
    {
        UE_LOG(LogAssimpModel, Warning, TEXT("No model loaded yet; cannot reimport."));
        return;
    }

    LoadModelFromFile(LoadedModelPath);
}

void AImportedModelActor::SetScaleFactor(float NewScale)
{
    if (!FMath::IsNearlyEqual(ScaleFactor, NewScale))
    {
        ScaleFactor = NewScale;
        Reimport();
    }
}

void AImportedModelActor::SetRotationCorrection(FRotator NewRotation)
{
    if (!RotationCorrection.Equals(NewRotation))
    {
        RotationCorrection = NewRotation;
        Reimport();
    }
}

FString AImportedModelActor::GetModelDirectory() const
{
    return FPaths::GetPath(LoadedModelPath);
}

UTexture2D* AImportedModelActor::LoadTextureFromFile(const FString& FilePath)
{
    if (!FPaths::FileExists(FilePath))
    {
        UE_LOG(LogAssimpModel, Error, TEXT("Texture file not found: %s"), *FilePath);
        return nullptr;
    }

    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        UE_LOG(LogAssimpModel, Error, TEXT("Failed to load texture file data: %s"), *FilePath);
        return nullptr;
    }

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);

    if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
    {
        TArray64<uint8> UncompressedRGBA;
        if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedRGBA))
        {
            UTexture2D* Texture = UTexture2D::CreateTransient(
                ImageWrapper->GetWidth(),
                ImageWrapper->GetHeight(),
                PF_B8G8R8A8
            );

            if (!Texture)
            {
                UE_LOG(LogAssimpModel, Error, TEXT("Failed to create transient texture."));
                return nullptr;
            }

            void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
            FMemory::Memcpy(TextureData, UncompressedRGBA.GetData(), UncompressedRGBA.Num());
            Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

            Texture->UpdateResource();

            // Optional: set texture settings
            Texture->SRGB = true;

            return Texture;
        }
    }

    UE_LOG(LogAssimpModel, Error, TEXT("Failed to decode texture image: %s"), *FilePath);
    return nullptr;
}

void AImportedModelActor::LoadAssimpScene(const aiScene* Scene)
{
    ProcMesh->ClearAllMeshSections();

    const FTransform Transform(RotationCorrection, FVector::ZeroVector, FVector(ScaleFactor));

    unsigned int meshSection = 0;

    for (unsigned int j = 0; j < Scene->mNumMeshes; j++)
    {
        if (MeshSectionsToSkip.Contains(j))
        {
            UE_LOG(LogAssimpModel, Warning, TEXT("Skipping mesh section %d"), j);
            continue;
        }

        const aiMesh* Mesh = Scene->mMeshes[j];

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;

        // Process vertices
        for (unsigned int i = 0; i < Mesh->mNumVertices; i++)
        {
            aiVector3D Pos = Mesh->mVertices[i];
            FVector Vertex(Pos.x, Pos.y, Pos.z);
            Vertex = Transform.TransformPosition(Vertex);
            Vertices.Add(Vertex);

            aiVector3D Normal = Mesh->HasNormals() ? Mesh->mNormals[i] : aiVector3D(0, 0, 1);
            FVector NormalVec(Normal.x, Normal.y, Normal.z);
            NormalVec = Transform.TransformVector(NormalVec);
            Normals.Add(NormalVec);

            if (Mesh->HasTextureCoords(0))
            {
                aiVector3D UV = Mesh->mTextureCoords[0][i];
                UVs.Add(FVector2D(UV.x, 1.0f - UV.y)); // Flip V
            }
            else
            {
                UVs.Add(FVector2D(0.f, 0.f));
            }
        }

        // Process triangles (faces)
        for (unsigned int i = 0; i < Mesh->mNumFaces; i++)
        {
            const aiFace& Face = Mesh->mFaces[i];
            if (Face.mNumIndices == 3)
            {
                // Flip winding order for Unreal (clockwise -> counter-clockwise)
                Triangles.Add(Face.mIndices[0]);
                Triangles.Add(Face.mIndices[2]);
                Triangles.Add(Face.mIndices[1]);
            }
        }

        ProcMesh->CreateMeshSection_LinearColor(meshSection++, Vertices, Triangles, Normals, UVs, {}, {}, true);

        // Load and apply material + texture
        UMaterialInstanceDynamic* DynMaterial = nullptr;

        if (BaseMaterial && Mesh->mMaterialIndex >= 0 && Scene->HasMaterials())
        {
            aiMaterial* Material = Scene->mMaterials[Mesh->mMaterialIndex];

            aiString TexturePath;
            if (Material->GetTexture(aiTextureType_DIFFUSE, 0, &TexturePath) == AI_SUCCESS)
            {
                FString TextureFileName = UTF8_TO_TCHAR(TexturePath.C_Str());
                FString TextureFilePath;

                if (FPaths::IsRelative(TextureFileName))
                {
                    TextureFilePath = FPaths::Combine(GetModelDirectory(), TextureFileName);
                }
                else
                {
                    TextureFilePath = TextureFileName;
                }

                UTexture2D* LoadedTexture = LoadTextureFromFile(TextureFilePath);

                if (LoadedTexture)
                {
                    DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                    DynMaterial->SetTextureParameterValue(FName("BaseTexture"), LoadedTexture);
                }
            }
        }

        if (DynMaterial)
        {
            ProcMesh->SetMaterial(j, DynMaterial);
        }
    }
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProcMesh->SetCollisionObjectType(ECC_WorldDynamic);
    ProcMesh->SetCollisionResponseToAllChannels(ECR_Block);

    // Ensure collision data is cooked
    ProcMesh->ContainsPhysicsTriMeshData(true);
    ProcMesh->bUseComplexAsSimpleCollision = true;
}

void AImportedModelActor::SetMeshSectionsToSkip(const TSet<int32>& NewMeshSectionsToSkip)
{
    if (!MeshSectionsToSkip.Includes(NewMeshSectionsToSkip) || !NewMeshSectionsToSkip.Includes(MeshSectionsToSkip))
    {
        MeshSectionsToSkip = NewMeshSectionsToSkip;
        Reimport();
    }
}

#if WITH_EDITOR
void AImportedModelActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(AImportedModelActor, ScaleFactor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AImportedModelActor, RotationCorrection) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AImportedModelActor, MeshSectionsToSkip))
    {
        Reimport();
    }
}
#endif