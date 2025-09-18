#pragma once
#include "CoreMinimal.h"
#include "FGraphicsSettings.generated.h"

USTRUCT(BlueprintType)
struct FGraphicsSettings
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Version = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 GlobalIlluminationQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 ShadowQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 ShadingQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 TextureQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 ReflectionQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	TEnumAsByte<EWindowMode::Type> WindowMode;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	float FrameRateLimit;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	FIntPoint ScreenResolution;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bVSync;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	float ResolutionScaleValue;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 AntiAliasingQuality;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	FPostProcessSettings PostProcessSettings;

	enum class EFieldID : uint32
	{
		Version = 1,
		GlobalIlluminationQuality = 2,
		ShadowQuality = 3,
		ShadingQuality = 4,
		TextureQuality = 5,
		ReflectionQuality = 6,
		WindowMode = 7,
		FrameRateLimit = 8,
		ScreenResolution = 9,
		bVSync = 10,
		ResolutionScaleValue = 11,
		AntiAliasingQuality = 12,
		PostProcessSettings = 13,
	};


	FGraphicsSettings() : GlobalIlluminationQuality(3), ShadowQuality(3), ShadingQuality(3), TextureQuality(3),
	                      ReflectionQuality(3), WindowMode(EWindowMode::Type::Fullscreen),
	                      FrameRateLimit(240.0f),
	                      ScreenResolution(FIntPoint(1920, 1080)),
	                      bVSync(false),
	                      ResolutionScaleValue(100),
	                      AntiAliasingQuality(3)
	{
	}

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
		WriteTaggedChunk(EFieldID::GlobalIlluminationQuality, &GlobalIlluminationQuality, sizeof(int32));
		WriteTaggedChunk(EFieldID::ShadowQuality, &ShadowQuality, sizeof(int32));
		WriteTaggedChunk(EFieldID::ShadingQuality, &ShadingQuality, sizeof(int32));
		WriteTaggedChunk(EFieldID::TextureQuality, &TextureQuality, sizeof(int32));
		WriteTaggedChunk(EFieldID::ReflectionQuality, &ReflectionQuality, sizeof(int32));
		WriteTaggedChunk(EFieldID::WindowMode, &WindowMode, sizeof(EWindowMode::Type));
		WriteTaggedChunk(EFieldID::FrameRateLimit, &FrameRateLimit, sizeof(float));
		WriteTaggedChunk(EFieldID::ScreenResolution, &ScreenResolution, sizeof(FIntPoint));
		WriteTaggedChunk(EFieldID::bVSync, &bVSync, sizeof(bool));
		WriteTaggedChunk(EFieldID::ResolutionScaleValue, &ResolutionScaleValue, sizeof(float));
		WriteTaggedChunk(EFieldID::AntiAliasingQuality, &AntiAliasingQuality, sizeof(int32));
		WriteTaggedChunk(EFieldID::PostProcessSettings, &PostProcessSettings, sizeof(FPostProcessSettings));

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
				UE_LOG(LogTemp, Warning, TEXT("FGraphicsSettings: Invalid chunk size %d at offset %d"), Size, Offset);
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

			case EFieldID::GlobalIlluminationQuality:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&GlobalIlluminationQuality, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::ShadowQuality:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&ShadowQuality, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::ShadingQuality:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&ShadingQuality, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::TextureQuality:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&TextureQuality, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::ReflectionQuality:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&ReflectionQuality, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::WindowMode:
				if (Size == sizeof(EWindowMode::Type))
				{
					FMemory::Memcpy(&WindowMode, ChunkData, sizeof(EWindowMode::Type));
				}
				break;

			case EFieldID::FrameRateLimit:
				if (Size == sizeof(float))
				{
					FMemory::Memcpy(&FrameRateLimit, ChunkData, sizeof(float));
				}
				break;

			case EFieldID::ScreenResolution:
				if (Size == sizeof(FIntPoint))
				{
					FMemory::Memcpy(&ScreenResolution, ChunkData, sizeof(FIntPoint));
				}
				break;

			case EFieldID::bVSync:
				if (Size == sizeof(bool))
				{
					FMemory::Memcpy(&bVSync, ChunkData, sizeof(bool));
				}
				break;

			case EFieldID::ResolutionScaleValue:
				if (Size == sizeof(float))
				{
					FMemory::Memcpy(&ResolutionScaleValue, ChunkData, sizeof(float));
				}
				break;

			case EFieldID::AntiAliasingQuality:
				if (Size == sizeof(int32))
				{
					FMemory::Memcpy(&AntiAliasingQuality, ChunkData, sizeof(int32));
				}
				break;

			case EFieldID::PostProcessSettings:
				if (Size == sizeof(FPostProcessSettings))
				{
					FMemory::Memcpy(&PostProcessSettings, ChunkData, sizeof(FPostProcessSettings));
				}
				break;

			default:
				// Unknown field - skip silently for forwards compatibility
				UE_LOG(LogTemp, Log, TEXT("FGraphicsSettings: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
				break;
			}
		}
	}
};
