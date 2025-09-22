#pragma once

#include "CoreMinimal.h"
#include "FHotbarSlotState.h"
#include "FHotbarState.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FHotbarState
{
	GENERATED_BODY()

	// Field IDs for tagged chunks
	enum class EFieldID : uint32
	{
		HotbarOverride = 1,
		SelectedSlot = 2
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<uint8, FHotbarSlotState> HotbarOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 SelectedSlot = 0;

	// Tagged chunk serialization method
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
		
		// Serialize the HotbarOverride map
		WriteTaggedChunk(EFieldID::HotbarOverride, SerializeHotbarOverride());
		WriteTaggedChunk(EFieldID::SelectedSlot, TArray<uint8>{&SelectedSlot, sizeof(uint8)});

		return ByteArray;
	}

	// Tagged chunk deserialization method
	void DeserializeFromBytes(const TArray<uint8>& ByteArray)
	{
		if (ByteArray.Num() < sizeof(uint32) + sizeof(uint32))
		{
			return; // Invalid data
		}
		
		int32 Offset = 0;
		
		// Read tagged chunks until we reach the end
		while (Offset + sizeof(uint32) + sizeof(uint32) <= ByteArray.Num())
		{
			// Read field ID and size
			const uint32 FieldID = *reinterpret_cast<const uint32*>(ByteArray.GetData() + Offset);
			Offset += sizeof(uint32);
			
			const uint32 Size = *reinterpret_cast<const uint32*>(ByteArray.GetData() + Offset);
			Offset += sizeof(uint32);
			
			// Validate size doesn't exceed remaining data
			if (Offset + static_cast<int32>(Size) > ByteArray.Num())
			{
				UE_LOG(LogTemp, Warning, TEXT("FHotbarState: Invalid chunk size %d at offset %d"), Size, Offset);
				break;
			}
			
			// Extract chunk payload
			const TArray<uint8> ChunkData(ByteArray.GetData() + Offset, Size);
			Offset += Size;
			
			// Process known field types
			const EFieldID TypedFieldID = static_cast<EFieldID>(FieldID);
			switch (TypedFieldID)
			{
			case EFieldID::HotbarOverride:
				if (ChunkData.Num() > 0)
				{
					DeserializeHotbarOverride(ChunkData);
				}
				break;

			case EFieldID::SelectedSlot:
				SelectedSlot = ChunkData[0];
				break;

			default:
				// Unknown field - skip silently for forwards compatibility
				UE_LOG(LogTemp, Log, TEXT("FHotbarState: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
				break;
			}
		}
	}

private:
	// Helper method to serialize the HotbarOverride map
	TArray<uint8> SerializeHotbarOverride() const
	{
		TArray<uint8> MapData;

		// Calculate required size for map structure
		const int32 MapSize = HotbarOverride.Num();
		
		// Start with map size
		MapData.Append(reinterpret_cast<const uint8*>(&MapSize), sizeof(int32));

		// Write each key-value pair using the new tagged serialization
		for (const auto& Pair : HotbarOverride)
		{
			// Write the key
			MapData.Append(reinterpret_cast<const uint8*>(&Pair.Key), sizeof(uint8));

			// Serialize the value using its new SerializeToBytes method
			const TArray<uint8> ValueBytes = Pair.Value.SerializeToBytes();
			const int32 ValueSize = ValueBytes.Num();
			
			// Write value size then value data
			MapData.Append(reinterpret_cast<const uint8*>(&ValueSize), sizeof(int32));
			MapData.Append(ValueBytes);
		}

		return MapData;
	}

	// Helper method to deserialize the HotbarOverride map
	void DeserializeHotbarOverride(const TArray<uint8>& MapData)
	{
		if (MapData.Num() < sizeof(int32))
		{
			return; // Invalid data
		}

		const uint8* DataPtr = MapData.GetData();
		int32 Offset = 0;
		HotbarOverride.Empty();

		// Read map size
		int32 MapSize;
		FMemory::Memcpy(&MapSize, DataPtr + Offset, sizeof(int32));
		Offset += sizeof(int32);

		// Read each key-value pair
		for (int32 i = 0; i < MapSize; ++i)
		{
			// Validate we have enough data for key
			if (Offset + sizeof(uint8) + sizeof(int32) > MapData.Num())
			{
				UE_LOG(LogTemp, Warning, TEXT("FHotbarState: Insufficient data for map entry %d"), i);
				break;
			}

			// Read the key
			uint8 Key;
			FMemory::Memcpy(&Key, DataPtr + Offset, sizeof(uint8));
			Offset += sizeof(uint8);

			// Read value size
			int32 ValueSize;
			FMemory::Memcpy(&ValueSize, DataPtr + Offset, sizeof(int32));
			Offset += sizeof(int32);

			// Validate value size
			if (ValueSize <= 0 || Offset + ValueSize > MapData.Num())
			{
				UE_LOG(LogTemp, Warning, TEXT("FHotbarState: Invalid value size %d for key %d"), ValueSize, Key);
				break;
			}

			// Extract value data and deserialize
			const TArray<uint8> ValueBytes(DataPtr + Offset, ValueSize);
			Offset += ValueSize;

			FHotbarSlotState Value;
			Value.DeserializeFromBytes(ValueBytes);

			HotbarOverride.Add(Key, Value);
		}
	}
};