
#pragma once

#include "CoreMinimal.h"
#include "Runtime/Core/Public/UObject/NameTypes.h"
#include "Shared/Types/Enums/GameObjects/GameObjectType.h"
#include "FHotbarSlotState.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FHotbarSlotState
{
	GENERATED_BODY()

	// Field IDs for tagged chunks
	enum class EFieldID : uint32
	{
		ObjectType = 1,
		VoxelType  = 2,
		ObjectID   = 3,
		// Add new fields here with incrementing IDs
		// NewField = 3,
		// AnotherField = 4,
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar|State")
	EGameObjectType ObjectType = EGameObjectType::Voxel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar|State")
	uint8 VoxelType = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotbar|State")
	FName ObjectID;

	FORCEINLINE bool operator==(const FHotbarSlotState& Other) const
	{
		return ObjectType == Other.ObjectType && VoxelType == Other.VoxelType && ObjectID == Other.ObjectID;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FHotbarSlotState& State)
	{
		uint32 Hash = ::GetTypeHash(State.ObjectType);
		Hash = HashCombine(Hash, ::GetTypeHash(State.VoxelType));
		Hash = HashCombine(Hash, GetTypeHash(State.ObjectID));
		return Hash;
	}

	// Tagged chunk serialization
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
		
		// Serialize each field as a chunk
		WriteTaggedChunk(EFieldID::ObjectType, SerializeObjectTypeField(ObjectType));
		WriteTaggedChunk(EFieldID::VoxelType, SerializeUInt8Field(VoxelType));
		WriteTaggedChunk(EFieldID::ObjectID, SerializeFNameField(ObjectID));
		
		return ByteArray;
	}

	// Tagged chunk deserialization
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
				UE_LOG(LogTemp, Warning, TEXT("FHotbarSlotState: Invalid chunk size %d at offset %d"), Size, Offset);
				break;
			}
			
			// Extract chunk payload
			const TArray<uint8> ChunkData(ByteArray.GetData() + Offset, Size);
			Offset += Size;
			
			// Process known field types
			const EFieldID TypedFieldID = static_cast<EFieldID>(FieldID);
			switch (TypedFieldID)
			{
				case EFieldID::ObjectType:
					if (ChunkData.Num() > 0)
					{
						DeserializeObjectTypeField(ChunkData, ObjectType);
					}
					break;
					
				case EFieldID::VoxelType:
					if (ChunkData.Num() > 0)
					{
						DeserializeUInt8Field(ChunkData, VoxelType);
					}
					break;
					
				case EFieldID::ObjectID:
					if (ChunkData.Num() > 0)
					{
						DeserializeFNameField(ChunkData, ObjectID);
					}
					break;
					
				default:
					// Unknown field - skip silently for forwards compatibility
					UE_LOG(LogTemp, Log, TEXT("FHotbarSlotState: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
					break;
			}
		}
	}

	// Legacy Serialize method for FArchive compatibility
	bool Serialize(FArchive& Ar)
	{
		Ar << ObjectType;
		Ar << VoxelType;
		Ar << ObjectID;
		return true;
	}

	friend FArchive& operator<<(FArchive& Ar, FHotbarSlotState& State)
	{
		State.Serialize(Ar);
		return Ar;
	}

private:
	// Helper methods for individual field serialization
	TArray<uint8> SerializeObjectTypeField(EGameObjectType Value) const
	{
		TArray<uint8> Data;
		Data.SetNumUninitialized(sizeof(EGameObjectType));
		FMemory::Memcpy(Data.GetData(), &Value, sizeof(EGameObjectType));
		return Data;
	}

	TArray<uint8> SerializeFNameField(FName Value) const
	{
		FString NameString = Value.ToString();

		// Convert FString -> UTF-8 bytes
		FTCHARToUTF8 Converter(*NameString);
		const uint8* UTF8Data = reinterpret_cast<const uint8*>(Converter.Get());
		int32 Length = Converter.Length();

		TArray<uint8> Data;
		Data.SetNumUninitialized(sizeof(int32) + Length);

		// length
		FMemory::Memcpy(Data.GetData(), &Length, sizeof(int32));

		// string
		FMemory::Memcpy(Data.GetData() + sizeof(int32), UTF8Data, Length);

		return Data;
	}

	TArray<uint8> SerializeUInt8Field(uint8 Value) const
	{
		TArray<uint8> Data;
		Data.SetNumUninitialized(sizeof(uint8));
		FMemory::Memcpy(Data.GetData(), &Value, sizeof(uint8));
		return Data;
	}

	void DeserializeUInt8Field(const TArray<uint8>& Data, uint8& OutValue)
	{
		if (Data.Num() >= sizeof(uint8))
		{
			FMemory::Memcpy(&OutValue, Data.GetData(), sizeof(uint8));
		}
	}

	void DeserializeObjectTypeField(const TArray<uint8>& Data, EGameObjectType& OutValue)
	{
		if (Data.Num() >= sizeof(EGameObjectType))
		{
			FMemory::Memcpy(&OutValue, Data.GetData(), sizeof(EGameObjectType));
		}
	}

	void DeserializeFNameField(const TArray<uint8>& Data, FName& OutValue)
	{
		if (Data.Num() < sizeof(int32))
		{
			OutValue = NAME_None;
			UE_LOG(LogTemp, Warning, TEXT("FHotbarSlotState: Unable to read ObjectID length"));
			return;
		}

		// Read length
		int32 Length = 0;
		FMemory::Memcpy(&Length, Data.GetData(), sizeof(int32));

		if (Length <= 0 || sizeof(int32) + Length > Data.Num())
		{
			OutValue = NAME_None;
			UE_LOG(LogTemp, Warning, TEXT("FHotbarSlotState: Unable to read ObjectID"));
			return; // corrupted data
		}

		// Read UTF-8 string
		const ANSICHAR* UTF8String = reinterpret_cast<const ANSICHAR*>(Data.GetData() + sizeof(int32));
		FString NameString = FString(Length, UTF8String);

		OutValue = FName(*NameString);
	}
};