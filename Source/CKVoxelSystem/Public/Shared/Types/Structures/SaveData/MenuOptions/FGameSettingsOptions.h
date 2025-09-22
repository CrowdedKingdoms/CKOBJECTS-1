#pragma once
#include "CoreMinimal.h"
#include "Shared/Types/Enums/Character/EPlayerCameraView.h"
#include "Shared/Types/Enums/Character/EPlayerFlightControlScheme.h"
#include "Shared/Types/Enums/Character/E_MovmentMode.h"
#include "Shared/Types/Structures/Player/FPlayerLastKnownAddress.h"
#include "FGameSettingsOptions.generated.h"

USTRUCT(BlueprintType)
struct FGameSettingsOptions
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	int32 RenderDistance;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	int32 LoadDistance;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	float InteractRadius;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bChunkBoundaries;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bChunkBillboards;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bVoxelBillboards;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bPlayerAddress;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	float FOVSliderValue;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	float CameraDistanceSliderValue;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bAirCreate;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bPerformanceMetrics;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bShowCompass;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bShowMinimap;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	bool bShowVoxelHotbar;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	E_MovementMode PlayerMovementMode;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	EPlayerFlightControlScheme PlayerFlightControlScheme;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	EPlayerCameraView PlayerCameraView;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings Options")
	FPlayerLastKnownAddress PlayerLastKnownAddress;

	FGameSettingsOptions() : RenderDistance(5), LoadDistance(5), InteractRadius(5), bChunkBoundaries(false),
	                         bChunkBillboards(false),
	                         bVoxelBillboards(false),
	                         bPlayerAddress(true), FOVSliderValue(90.0f),
	                         CameraDistanceSliderValue(300.0f), bAirCreate(false),
	                         bPerformanceMetrics(false),
	                         bShowCompass(true), bShowMinimap(true),
	                         bShowVoxelHotbar(true), PlayerMovementMode(E_MovementMode::Parkour),
	                         PlayerFlightControlScheme(EPlayerFlightControlScheme::Simple),
	                         PlayerCameraView(EPlayerCameraView::ThirdPerson),
	                         PlayerLastKnownAddress(1, 0, 0, 2, 0, 0, 0)
	{
	}

	enum class EFieldID : uint32
	{
		Version = 1,
		RenderDistance = 2,
		LoadDistance = 3,
		InteractRadius = 4,
		bChunkBoundaries = 5,
		bChunkBillboards = 6,
		bVoxelBillboards = 7,
		bPlayerAddress = 8,
		FOVSliderValue = 9,
		CameraDistanceSliderValue = 10,
		bAirCreate = 11,
		bPerformanceMetrics = 12,
		bShowCompass = 13,
		bShowMinimap = 14,
		bShowVoxelHotbar = 15,
		PlayerMovementMode = 16,
		PlayerFlightControlScheme = 17,
		PlayerCameraView = 18,
		PlayerLastKnownAddress = 19,
	};


	TArray<uint8> SerializeToBytes() const
	{
		TArray<uint8> ByteArray;

		// Helper to write a tagged chunk: [ID:4][Size:4][Payload: Size]
		auto WriteTaggedChunk = [&ByteArray](EFieldID FieldID, const void* Data, const uint32 Size)
		{
			const uint32 ID = static_cast<uint32>(FieldID);

			ByteArray.Append(reinterpret_cast<const uint8*>(&ID), sizeof(uint32));
			ByteArray.Append(reinterpret_cast<const uint8*>(&Size), sizeof(uint32));
			ByteArray.Append(static_cast<const uint8*>(Data), Size);
		};

		// Write all field chunks
		WriteTaggedChunk(EFieldID::Version, &Version, sizeof(uint8));
		WriteTaggedChunk(EFieldID::RenderDistance, &RenderDistance, sizeof(int32));
		WriteTaggedChunk(EFieldID::LoadDistance, &LoadDistance, sizeof(int32));
		WriteTaggedChunk(EFieldID::InteractRadius, &InteractRadius, sizeof(float));
		WriteTaggedChunk(EFieldID::bChunkBoundaries, &bChunkBoundaries, sizeof(bool));
		WriteTaggedChunk(EFieldID::bChunkBillboards, &bChunkBillboards, sizeof(bool));
		WriteTaggedChunk(EFieldID::bVoxelBillboards, &bVoxelBillboards, sizeof(bool));
		WriteTaggedChunk(EFieldID::bPlayerAddress, &bPlayerAddress, sizeof(bool));
		WriteTaggedChunk(EFieldID::FOVSliderValue, &FOVSliderValue, sizeof(float));
		WriteTaggedChunk(EFieldID::CameraDistanceSliderValue, &CameraDistanceSliderValue, sizeof(float));
		WriteTaggedChunk(EFieldID::bAirCreate, &bAirCreate, sizeof(bool));
		WriteTaggedChunk(EFieldID::bPerformanceMetrics, &bPerformanceMetrics, sizeof(bool));
		WriteTaggedChunk(EFieldID::bShowCompass, &bShowCompass, sizeof(bool));
		WriteTaggedChunk(EFieldID::bShowMinimap, &bShowMinimap, sizeof(bool));
		WriteTaggedChunk(EFieldID::bShowVoxelHotbar, &bShowVoxelHotbar, sizeof(bool));
		WriteTaggedChunk(EFieldID::PlayerMovementMode, &PlayerMovementMode, sizeof(E_MovementMode));
		WriteTaggedChunk(EFieldID::PlayerFlightControlScheme, &PlayerFlightControlScheme,
		                 sizeof(EPlayerFlightControlScheme));
		WriteTaggedChunk(EFieldID::PlayerCameraView, &PlayerCameraView, sizeof(EPlayerCameraView));
		WriteTaggedChunk(EFieldID::PlayerLastKnownAddress, &PlayerLastKnownAddress, sizeof(FPlayerLastKnownAddress));

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
				UE_LOG(LogTemp, Warning, TEXT("FGameSettingsOptions: Invalid chunk size %d at offset %d"), Size,
				       Offset);
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

			case EFieldID::RenderDistance:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&RenderDistance, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::LoadDistance:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&LoadDistance, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::InteractRadius:
				if (Size == sizeof(float))
				{
					FMemory::Memcpy(&InteractRadius, ChunkData, sizeof(float));
				}
				break;

			case EFieldID::bChunkBoundaries:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bChunkBoundaries, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bChunkBillboards:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bChunkBillboards, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bVoxelBillboards:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bVoxelBillboards, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bPlayerAddress:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bPlayerAddress, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::FOVSliderValue:
				if (Size == sizeof(float))
				{
					FMemory::Memcpy(&FOVSliderValue, ChunkData, sizeof(float));
				}
				break;

			case EFieldID::CameraDistanceSliderValue:
				if (Size == sizeof(float))
				{
					FMemory::Memcpy(&CameraDistanceSliderValue, ChunkData, sizeof(float));
				}
				break;

			case EFieldID::bAirCreate:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bAirCreate, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bPerformanceMetrics:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bPerformanceMetrics, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bShowCompass:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bShowCompass, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bShowMinimap:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bShowMinimap, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::bShowVoxelHotbar:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bShowVoxelHotbar, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::PlayerMovementMode:
				if (Size == sizeof(E_MovementMode))
				{
					FMemory::Memcpy(&PlayerMovementMode, ChunkData, sizeof(E_MovementMode));
				}
				break;

			case EFieldID::PlayerFlightControlScheme:
				if (Size == sizeof(EPlayerFlightControlScheme))
				{
					FMemory::Memcpy(&PlayerFlightControlScheme, ChunkData, sizeof(EPlayerFlightControlScheme));
				}
				break;

			case EFieldID::PlayerCameraView:
				if (Size == sizeof(EPlayerCameraView))
				{
					FMemory::Memcpy(&PlayerCameraView, ChunkData, sizeof(EPlayerCameraView));
				}
				break;

			case EFieldID::PlayerLastKnownAddress:
				if (Size == sizeof(FPlayerLastKnownAddress))
				{
					FMemory::Memcpy(&PlayerLastKnownAddress, ChunkData, sizeof(FPlayerLastKnownAddress));
				}
				break;

			default:
				// Unknown field - skip silently for forwards compatibility
				UE_LOG(LogTemp, Log, TEXT("FGameSettingsOptions: Skipping unknown field ID %d (size: %d)"), FieldID,
				       Size);
				break;
			}
		}
	}
};
