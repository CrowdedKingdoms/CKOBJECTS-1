
#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Structures/Avatar/FColorSlotsSaveData.h"
#include "FCharacterCustomization.generated.h"

USTRUCT(BlueprintType)
struct FCharacterCustomization
{
	GENERATED_BODY()

	// Field IDs for tagged chunks
	enum class EFieldID : uint32
	{
		Version = 1,
		ColorSaveData = 2,
		AvatarID = 3
		// Add new fields here with incrementing IDs
		// NewField = 4,
	};

	UPROPERTY()
	uint8 Version = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Character Customization")
	FColorSlotsSaveData ColorSaveData;

	UPROPERTY(BlueprintReadWrite, Category = "Character Customization")
	int64 AvatarID;

	FCharacterCustomization() : AvatarID(0)
	{
	}

	TArray<uint8> SerializeToBytes() const
	{
		TArray<uint8> ByteArray;
		
		// Helper to write a tagged chunk: [ID:4][Size:4][Payload:Size]
		auto WriteTaggedChunk = [&ByteArray](EFieldID FieldID, const TArray<uint8>& Data) {
			const uint32 ID = static_cast<uint32>(FieldID);
			const uint32 Size = Data.Num();
			ByteArray.Append(reinterpret_cast<const uint8*>(&ID), sizeof(uint32));
			ByteArray.Append(reinterpret_cast<const uint8*>(&Size), sizeof(uint32));
			ByteArray.Append(Data);
		};
		
		// Write all field chunks
		WriteTaggedChunk(EFieldID::Version, TArray(&Version, sizeof(uint8)));
		WriteTaggedChunk(EFieldID::ColorSaveData, ColorSaveData.SerializeToBytes());
		WriteTaggedChunk(EFieldID::AvatarID, TArray(reinterpret_cast<const uint8*>(&AvatarID), sizeof(int64)));
		
		return ByteArray;
	}


	void DeserializeFromBytes(const TArray<uint8>& Data)
	{
		if (Data.Num() < sizeof(uint32) + sizeof(uint32))
		{
			return; // Invalid data
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
				UE_LOG(LogTemp, Warning, TEXT("FCharacterCustomization: Invalid chunk size %d at offset %d"), Size, Offset);
				break;
			}
			
			// Extract chunk payload
			const TArray<uint8> ChunkData(Data.GetData() + Offset, Size);
			Offset += Size;
			
			// Process known field types
			const EFieldID TypedFieldID = static_cast<EFieldID>(FieldID);
			switch (TypedFieldID)
			{
			case EFieldID::Version:
				if (ChunkData.Num() == sizeof(uint8))
				{
					FMemory::Memcpy(&Version, ChunkData.GetData(), sizeof(uint8));
				}
				break;
					
			case EFieldID::ColorSaveData:
				if (ChunkData.Num() > 0)
				{
					ColorSaveData.DeserializeFromBytes(ChunkData);
				}
				break;
					
			case EFieldID::AvatarID:
				if (ChunkData.Num() == sizeof(int64))
				{
					FMemory::Memcpy(&AvatarID, ChunkData.GetData(), sizeof(int64));
				}
				break;
					
			default:
				// Unknown field - skip silently for forwards compatibility
				UE_LOG(LogTemp, Log, TEXT("FCharacterCustomization: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
				break;
			}
		}
	}

};