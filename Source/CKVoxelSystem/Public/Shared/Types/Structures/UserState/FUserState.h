#pragma once

#include "CoreMinimal.h"
#include "FHotbarState.h"
#include "Shared/Types/Structures/SaveData/CharacterCustomization/FCharacterCustomization.h"
#include "Shared/Types/Structures/SaveData/ChatMenu/FTextChatOptions.h"
#include "Shared/Types/Structures/SaveData/GraphicsSettings/FGraphicsSettings.h"
#include "Shared/Types/Structures/SaveData/MenuOptions/FGameSettingsOptions.h"
#include "Shared/Types/Structures/SaveData/MenuOptions/FTeleportMenuOptions.h"
#include "FUserState.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FUserState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHotbarState HotbarState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCharacterCustomization CharacterCustomizationSaveGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTextChatOptions TextChatOptionsSaveGame;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGraphicsSettings GraphicsSettingsSaveGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameSettingsOptions GameSettingsOptionsSaveGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTeleportMenuOptions TeleportMenuOptionsSaveGame;


	enum class EFieldID : uint32
	{
		HotbarState = 1,
		CharacterCustomizationSaveGame = 2,
		TextChatOptionsSaveGame = 3,
		GraphicsSettingsSaveGame = 4,
		GameSettingsOptionsSaveGame = 5,
		TeleportMenuOptionsSaveGame = 6,
	};


	// Method to serialize to UTF8 String
	FString ToUTF8String() const
	{
		TArray<uint8> ByteArray;
		
		// Helper to write a tagged chunk: [ID:4][Size:4][Payload: Size]
		auto WriteTaggedChunk = [&ByteArray](EFieldID FieldID, const TArray<uint8>& Data) {
			const uint32 ID = static_cast<uint32>(FieldID);
			const uint32 Size = Data.Num();
			
			ByteArray.Append(reinterpret_cast<const uint8*>(&ID), sizeof(uint32));
			ByteArray.Append(reinterpret_cast<const uint8*>(&Size), sizeof(uint32));
			ByteArray.Append(Data);
		};
		
		// Write all field chunks
		WriteTaggedChunk(EFieldID::HotbarState, HotbarState.SerializeToBytes());
		WriteTaggedChunk(EFieldID::CharacterCustomizationSaveGame, CharacterCustomizationSaveGame.SerializeToBytes());
		WriteTaggedChunk(EFieldID::TextChatOptionsSaveGame, TextChatOptionsSaveGame.SerializeToBytes());
		WriteTaggedChunk(EFieldID::GraphicsSettingsSaveGame, GraphicsSettingsSaveGame.SerializeToBytes());
		WriteTaggedChunk(EFieldID::GameSettingsOptionsSaveGame, GameSettingsOptionsSaveGame.SerializeToBytes());
		WriteTaggedChunk(EFieldID::TeleportMenuOptionsSaveGame, TeleportMenuOptionsSaveGame.SerializeToBytes());
		
		return FBase64::Encode(ByteArray);

	}



	static FUserState FromUTF8String(const FString& Base64String)
	{
		FUserState Result;

		if (TArray<uint8> ByteArray; FBase64::Decode(Base64String, ByteArray))
		{
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
					UE_LOG(LogTemp, Warning, TEXT("FUserState: Invalid chunk size %d at offset %d"), Size, Offset);
					break;
				}
				
				// Extract chunk payload
				const TArray<uint8> ChunkData(ByteArray.GetData() + Offset, Size);
				Offset += Size;
				
				// Process known field types
				const EFieldID TypedFieldID = static_cast<EFieldID>(FieldID);
				switch (TypedFieldID)
				{
					case EFieldID::HotbarState:
						if (ChunkData.Num() > 0)
						{
							Result.HotbarState.DeserializeFromBytes(ChunkData);
						}
						break;
						
					case EFieldID::CharacterCustomizationSaveGame:
						if (ChunkData.Num() > 0)
						{
							Result.CharacterCustomizationSaveGame.DeserializeFromBytes(ChunkData);
						}
						break;
						
					case EFieldID::TextChatOptionsSaveGame:
						if (ChunkData.Num() > 0)
						{
							Result.TextChatOptionsSaveGame.DeserializeFromBytes(ChunkData);
						}
						break;
						
					case EFieldID::GraphicsSettingsSaveGame:
						if (ChunkData.Num() > 0)
						{
							Result.GraphicsSettingsSaveGame.DeserializeFromBytes(ChunkData);
						}
						break;
						
					case EFieldID::GameSettingsOptionsSaveGame:
						if (ChunkData.Num() > 0)
						{
							Result.GameSettingsOptionsSaveGame.DeserializeFromBytes(ChunkData);
						}
						break;
						
					case EFieldID::TeleportMenuOptionsSaveGame:
						if (ChunkData.Num() > 0)
						{
							Result.TeleportMenuOptionsSaveGame.DeserializeFromBytes(ChunkData);
						}
						break;
						
					default:
						// Unknown field - skip silently for forwards compatibility
						UE_LOG(LogTemp, Log, TEXT("FUserState: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
						break;
				}
			}
		}

		return Result;
	}
};
