#pragma once
#include "CoreMinimal.h"
#include "FTeleportMenuOptions.generated.h"

USTRUCT(BlueprintType)
struct FTeleportMenuOptions
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Teleport Menu Options")
	int64 ChunkX;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Teleport Menu Options")
	int64 ChunkY;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Teleport Menu Options")
	int64 ChunkZ;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Teleport Menu Options")
	int64 VoxelX;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Teleport Menu Options")
	int64 VoxelY;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Teleport Menu Options")
	int64 VoxelZ;

	FTeleportMenuOptions() : ChunkX(0), ChunkY(0), ChunkZ(0), VoxelX(0), VoxelY(0), VoxelZ(0)
	{
	}

	enum class EFieldID : uint32
	{
		Version = 1,
		ChunkX = 2,
		ChunkY = 3,
		ChunkZ = 4,
		VoxelX = 5,
		VoxelY = 6,
		VoxelZ = 7,
	};


	TArray<uint8> SerializeToBytes() const
	{
		TArray<uint8> ByteArray;
		
		// Helper to write a tagged chunk: [ID:4][Size:4][Payload: Size]
		auto WriteTaggedChunk = [&ByteArray](EFieldID FieldID, const void* Data, const uint32 Size) {
			const uint32 ID = static_cast<uint32>(FieldID);
			
			ByteArray.Append(reinterpret_cast<const uint8*>(&ID), sizeof(uint32));
			ByteArray.Append(reinterpret_cast<const uint8*>(&Size), sizeof(uint32));
			ByteArray.Append(static_cast<const uint8*>(Data), Size);
		};
		
		// Write all field chunks
		WriteTaggedChunk(EFieldID::Version, &Version, sizeof(uint8));
		WriteTaggedChunk(EFieldID::ChunkX, &ChunkX, sizeof(int64));
		WriteTaggedChunk(EFieldID::ChunkY, &ChunkY, sizeof(int64));
		WriteTaggedChunk(EFieldID::ChunkZ, &ChunkZ, sizeof(int64));
		WriteTaggedChunk(EFieldID::VoxelX, &VoxelX, sizeof(int64));
		WriteTaggedChunk(EFieldID::VoxelY, &VoxelY, sizeof(int64));
		WriteTaggedChunk(EFieldID::VoxelZ, &VoxelZ, sizeof(int64));

		return ByteArray;
	}


	void DeserializeFromBytes(const TArray<uint8>& Data)
	{
		if (Data.Num() == 0)
		{
			return;
		}

		int32 Offset = 0;
		
		// Read tagged chunks until we reach the end
		while (Offset + sizeof(uint32) + sizeof(uint32) <= Data.Num())
		{
			// Read field ID and size
			const uint32 FieldID = *reinterpret_cast<const uint32*>(Data.GetData() + Offset);
			Offset += sizeof(uint32);
			
			const uint32 Size = *reinterpret_cast<const uint32*>(Data.GetData() + Offset);
			Offset += sizeof(uint32);
			
			// Validate size doesn't exceed remaining data
			if (Offset + static_cast<int32>(Size) > Data.Num())
			{
				UE_LOG(LogTemp, Warning, TEXT("FTeleportMenuOptions: Invalid chunk size %d at offset %d"), Size, Offset);
				break;
			}
			
			// Extract chunk payload
			const uint8* ChunkData = Data.GetData() + Offset;
			Offset += Size;
			
			// Process known field types
			const EFieldID TypedFieldID = static_cast<EFieldID>(FieldID);
			switch (TypedFieldID)
			{
				case EFieldID::Version:
					if (Size == sizeof(uint8))
					{
						FMemory::Memcpy(&Version, ChunkData, sizeof(uint8));
					}
					break;
					
				case EFieldID::ChunkX:
					if (Size == sizeof(int64))
					{
						FMemory::Memcpy(&ChunkX, ChunkData, sizeof(int64));
					}
					break;
					
				case EFieldID::ChunkY:
					if (Size == sizeof(int64))
					{
						FMemory::Memcpy(&ChunkY, ChunkData, sizeof(int64));
					}
					break;
					
				case EFieldID::ChunkZ:
					if (Size == sizeof(int64))
					{
						FMemory::Memcpy(&ChunkZ, ChunkData, sizeof(int64));
					}
					break;
					
				case EFieldID::VoxelX:
					if (Size == sizeof(int64))
					{
						FMemory::Memcpy(&VoxelX, ChunkData, sizeof(int64));
					}
					break;
					
				case EFieldID::VoxelY:
					if (Size == sizeof(int64))
					{
						FMemory::Memcpy(&VoxelY, ChunkData, sizeof(int64));
					}
					break;
					
				case EFieldID::VoxelZ:
					if (Size == sizeof(int64))
					{
						FMemory::Memcpy(&VoxelZ, ChunkData, sizeof(int64));
					}
					break;
					
				default:
					// Unknown field - skip silently for forwards compatibility
					UE_LOG(LogTemp, Log, TEXT("FTeleportMenuOptions: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
					break;
			}
		}
	}

};
