#pragma once
#include "CoreMinimal.h"
#include "CKNetwork/Pubilc/FunctionLibraries/Network/FL_Serialization.h"
#include "FTextChatOptions.generated.h"

USTRUCT(BlueprintType)
struct FTextChatOptions
{
	GENERATED_BODY()

	// Field IDs for tagged chunks
	enum class EFieldID : uint32
	{
		Version = 1,
		Username = 2
		// Add new fields here with incrementing IDs
		// NewField = 3,
	};

	UPROPERTY()
	uint8 Version = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Text Chat Options")
	FString Username;

	FTextChatOptions() : Username("")
	{
		
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
		
		// Write all field chunks
		WriteTaggedChunk(EFieldID::Version, TArray(&Version, sizeof(uint8)));
		WriteTaggedChunk(EFieldID::Username, SerializeString(Username));
		
		return ByteArray;
	}

	// Tagged chunk deserialization
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
				UE_LOG(LogTemp, Warning, TEXT("FTextChatOptions: Invalid chunk size %d at offset %d"), Size, Offset);
				break;
			}
			
			// Extract chunk payload
			const TArray ChunkData(Data.GetData() + Offset, Size);
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
					
				case EFieldID::Username:
					if (ChunkData.Num() > 0)
					{
						Username = DeserializeString(ChunkData);
					}
					break;
					
				default:
					// Unknown field - skip silently for forwards compatibility
					UE_LOG(LogTemp, Log, TEXT("FTextChatOptions: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
					break;
			}
		}
	}

private:
	// Helper method to serialize string
	TArray<uint8> SerializeString(const FString& Str) const
	{
		TArray<uint8> StringData;
		
		// Convert string to UTF-8
		FTCHARToUTF8 UTF8String(*Str);
		const int32 StringLength = UTF8String.Length();
		
		// Write string length first (4 bytes)
		StringData.Append(reinterpret_cast<const uint8*>(&StringLength), sizeof(int32));
		
		// Write string data
		if (StringLength > 0)
		{
			StringData.Append(reinterpret_cast<const uint8*>(UTF8String.Get()), StringLength);
		}
		
		return StringData;
	}

	// Helper method to deserialize string
	FString DeserializeString(const TArray<uint8>& Data) const
	{
		if (Data.Num() < sizeof(int32))
		{
			return "";
		}
		
		int32 Offset = 0;
		
		// Read string length
		int32 StringLength = 0;
		FMemory::Memcpy(&StringLength, Data.GetData() + Offset, sizeof(int32));
		Offset += sizeof(int32);
		
		// Validate string length
		if (StringLength < 0 || Offset + StringLength > Data.Num())
		{
			return "";
		}
		
		// Read string data
		if (StringLength > 0)
		{
			return UFL_Serialization::DeserializeString(Data, Offset, StringLength);
		}
		
		return "";
	}
};