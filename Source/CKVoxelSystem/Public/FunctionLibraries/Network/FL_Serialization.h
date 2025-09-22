// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#include "openssl/hmac.h"
THIRD_PARTY_INCLUDES_END
#undef UI


#include "FL_Serialization.generated.h"



/**
 * 
 */
UCLASS()
class  UFL_Serialization : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:
	
	template<typename T>
	static const TArray<uint8>& SerializeValue(T Value);

	template<typename T>
	static bool DeserializeValue(const TArray<uint8>& Data, T& OutValue, int Offset = 0);


	static FString DeserializeString(const TArray<uint8>& Payload, int32 Offset, int32 Length);
	static int32 DeserializeInt32(const TArray<uint8>& Payload, int32 Offset);
	static int64 DeserializeInt64(const TArray<uint8>& Payload, int32 Offset);
	static float DeserializeFloat(const TArray<uint8>& Payload, int32 Offset);

	static TArray<uint8> CalculateHMAC(const TArray<uint8>& Payload, const FString& GameToken);
	static bool AuthenticateHMAC(const TArray<uint8>& ReceivedMessage, const FString& GameToken);


	static bool ExtractChunkCoordinates(const TSharedPtr<FJsonObject>& JsonObj, int64& X, int64& Y, int64& Z);
	
};


template <typename T>
const TArray<uint8>& UFL_Serialization::SerializeValue(T Value)
{
	static TArray<uint8> SerializedArray;
	SerializedArray.SetNumUninitialized(sizeof(T));

	// Direct memory copy for little-endian to little-endian
	FMemory::Memcpy(SerializedArray.GetData(), &Value, sizeof(T));

	return SerializedArray;
}

template <typename T>
bool UFL_Serialization::DeserializeValue(const TArray<uint8>& Data, T& OutValue, const int Offset)
{
	static_assert(std::is_integral_v<T>, "T must be an integral type.");

	// Ensure there are enough bytes in the array
	if (Data.Num() < sizeof(T) + Offset)
	{
		UE_LOG(LogTemp, Error, TEXT("Not enough data to deserialize %s."), *FString(__FUNCTION__));
		return false;  // Indicate failure to deserialize
	}

	// Directly copy the bytes for little endian
	std::memcpy(&OutValue, Data.GetData() + Offset, sizeof(T));

	return true;  // Indicate success
}