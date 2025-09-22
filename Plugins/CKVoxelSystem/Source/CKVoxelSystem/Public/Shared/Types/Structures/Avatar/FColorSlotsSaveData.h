#pragma once

#include "CoreMinimal.h"
#include "FColorSlotsSaveData.generated.h"

USTRUCT(BlueprintType)
struct FColorSlotsSaveData
{
	GENERATED_BODY()

	// Field IDs for tagged chunks
	enum class EFieldID : uint32
	{
		Version = 1,
		HeadColor = 2,
		NeckBandColor = 3,
		ChestColor = 4,
		PelvisColor = 5,
		PelvisBandColor = 6,
		HandLeftColor = 7,
		HandRightColor = 8,
		UpperThighJointColor = 9,
		ClavicleLeftColor = 10,
		ClavicleRightColor = 11,
		ThighLeftColor = 12,
		ThighRightColor = 13,
		KneeLeftColor = 14,
		KneeRightColor = 15,
		UpperArmLeftColor = 16,
		UpperArmRightColor = 17,
		LegLeftColor = 18,
		LegRightColor = 19,
		FootJointLeftColor = 20,
		FootJointRightColor = 21,
		ArmJointLeftColor = 22,
		ArmJointRightColor = 23,
		FootLeftColor = 24,
		FootRightColor = 25,
		LowerArmLeftColor = 26,
		LowerArmRightColor = 27,
		HandJointLeftColor = 28,
		HandJointRightColor = 29
		// Add new fields here with incrementing IDs
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Version;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HeadColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor NeckBandColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ChestColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor PelvisColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor PelvisBandColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HandLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HandRightColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor UpperThighJointColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ClavicleLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ClavicleRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ThighLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ThighRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor KneeLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor KneeRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor UpperArmLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor UpperArmRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LegLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LegRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FootJointLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FootJointRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ArmJointLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ArmJointRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FootLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor FootRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LowerArmLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LowerArmRightColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HandJointLeftColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor HandJointRightColor;

	// Default constructor
	FColorSlotsSaveData()
		: Version(1)
		, HeadColor(FLinearColor::White)
		, NeckBandColor(FLinearColor::White)
		, ChestColor(FLinearColor::White)
		, PelvisColor(FLinearColor::White)
		, PelvisBandColor(FLinearColor::White)
		, HandLeftColor(FLinearColor::White)
		, HandRightColor(FLinearColor::White)
		, UpperThighJointColor(FLinearColor::White)
		, ClavicleLeftColor(FLinearColor::White)
		, ClavicleRightColor(FLinearColor::White)
		, ThighLeftColor(FLinearColor::White)
		, ThighRightColor(FLinearColor::White)
		, KneeLeftColor(FLinearColor::White)
		, KneeRightColor(FLinearColor::White)
		, UpperArmLeftColor(FLinearColor::White)
		, UpperArmRightColor(FLinearColor::White)
		, LegLeftColor(FLinearColor::White)
		, LegRightColor(FLinearColor::White)
		, FootJointLeftColor(FLinearColor::White)
		, FootJointRightColor(FLinearColor::White)
		, ArmJointLeftColor(FLinearColor::White)
		, ArmJointRightColor(FLinearColor::White)
		, FootLeftColor(FLinearColor::White)
		, FootRightColor(FLinearColor::White)
		, LowerArmLeftColor(FLinearColor::White)
		, LowerArmRightColor(FLinearColor::White)
		, HandJointLeftColor(FLinearColor::White)
		, HandJointRightColor(FLinearColor::White)
	{
	}
	
	// Tagged chunk serialization
	TArray<uint8> SerializeToBytes() const
	{
		TArray<uint8> ByteArray;
		
		// Helper to write a tagged chunk: [ID:4][Size:4][Payload:Size]
		auto WriteTaggedChunk = [&ByteArray](EFieldID FieldID, const void* Data, uint32 Size) {
			const uint32 ID = static_cast<uint32>(FieldID);
			ByteArray.Append(reinterpret_cast<const uint8*>(&ID), sizeof(uint32));
			ByteArray.Append(reinterpret_cast<const uint8*>(&Size), sizeof(uint32));
			ByteArray.Append(reinterpret_cast<const uint8*>(Data), Size);
		};
		
		// Write all field chunks
		WriteTaggedChunk(EFieldID::Version, &Version, sizeof(uint8));
		WriteTaggedChunk(EFieldID::HeadColor, &HeadColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::NeckBandColor, &NeckBandColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ChestColor, &ChestColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::PelvisColor, &PelvisColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::PelvisBandColor, &PelvisBandColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::HandLeftColor, &HandLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::HandRightColor, &HandRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::UpperThighJointColor, &UpperThighJointColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ClavicleLeftColor, &ClavicleLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ClavicleRightColor, &ClavicleRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ThighLeftColor, &ThighLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ThighRightColor, &ThighRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::KneeLeftColor, &KneeLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::KneeRightColor, &KneeRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::UpperArmLeftColor, &UpperArmLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::UpperArmRightColor, &UpperArmRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::LegLeftColor, &LegLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::LegRightColor, &LegRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::FootJointLeftColor, &FootJointLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::FootJointRightColor, &FootJointRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ArmJointLeftColor, &ArmJointLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::ArmJointRightColor, &ArmJointRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::FootLeftColor, &FootLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::FootRightColor, &FootRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::LowerArmLeftColor, &LowerArmLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::LowerArmRightColor, &LowerArmRightColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::HandJointLeftColor, &HandJointLeftColor, sizeof(FLinearColor));
		WriteTaggedChunk(EFieldID::HandJointRightColor, &HandJointRightColor, sizeof(FLinearColor));
		
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
				UE_LOG(LogTemp, Warning, TEXT("FColorSlotsSaveData: Invalid chunk size %d at offset %d"), Size, Offset);
				break;
			}
			
			// Process known field types
			const EFieldID TypedFieldID = static_cast<EFieldID>(FieldID);
			switch (TypedFieldID)
			{
				case EFieldID::Version:
					if (Size == sizeof(uint8))
						FMemory::Memcpy(&Version, Data.GetData() + Offset, sizeof(uint8));
					break;
				case EFieldID::HeadColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&HeadColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::NeckBandColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&NeckBandColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ChestColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ChestColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::PelvisColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&PelvisColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::PelvisBandColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&PelvisBandColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::HandLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&HandLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::HandRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&HandRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::UpperThighJointColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&UpperThighJointColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ClavicleLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ClavicleLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ClavicleRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ClavicleRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ThighLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ThighLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ThighRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ThighRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::KneeLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&KneeLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::KneeRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&KneeRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::UpperArmLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&UpperArmLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::UpperArmRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&UpperArmRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::LegLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&LegLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::LegRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&LegRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::FootJointLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&FootJointLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::FootJointRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&FootJointRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ArmJointLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ArmJointLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::ArmJointRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&ArmJointRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::FootLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&FootLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::FootRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&FootRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::LowerArmLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&LowerArmLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::LowerArmRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&LowerArmRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::HandJointLeftColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&HandJointLeftColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
				case EFieldID::HandJointRightColor:
					if (Size == sizeof(FLinearColor))
						FMemory::Memcpy(&HandJointRightColor, Data.GetData() + Offset, sizeof(FLinearColor));
					break;
					
				default:
					// Unknown field - skip silently for forwards compatibility
					UE_LOG(LogTemp, Log, TEXT("FColorSlotsSaveData: Skipping unknown field ID %d (size: %d)"), FieldID, Size);
					break;
			}
			
			Offset += Size;
		}
	}
	
	// Method to serialize to UTF8 String
	FString ToUTF8String() const
	{
		return FBase64::Encode(SerializeToBytes());
	}

	static FColorSlotsSaveData FromUTF8String(const FString& Base64String)
	{
		FColorSlotsSaveData Result;

		if (TArray<uint8> ByteArray; FBase64::Decode(Base64String, ByteArray))
		{
			Result.DeserializeFromBytes(ByteArray);
		}
		
		return Result;
	}
};