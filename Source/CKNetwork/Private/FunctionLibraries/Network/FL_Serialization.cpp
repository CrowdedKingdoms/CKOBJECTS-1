// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibraries/Network/FL_Serialization.h"
#include <openssl/evp.h>


FString UFL_Serialization::DeserializeString(const TArray<uint8>& Payload, int32 Offset, int32 Length)
{
	if (Offset + Length > Payload.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Offset + Length exceeds Payload size"));
		return FString();
	}

	// Extract exactly `Length` bytes
	TArray<uint8> StringBytes;
	StringBytes.Append(Payload.GetData() + Offset, Length);

	// Convert to FString (UTF-8)
	FString Result = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(StringBytes.GetData())));

	// Trim in case of null characters
	Result = Result.Left(Length);

	return Result;
}

int32 UFL_Serialization::DeserializeInt32(const TArray<uint8>& Payload, const int32 Offset)
{
	int32 Value = 0;
	if (Offset + 4 <= Payload.Num())
	{
		FMemory::Memcpy(&Value, Payload.GetData() + Offset, 4);
	}
	return Value;
}

int64 UFL_Serialization::DeserializeInt64(const TArray<uint8>& Payload, const int32 Offset)
{
	int64 Value = 0;
	if (Offset + sizeof(int64) <= Payload.Num())
	{
		FMemory::Memcpy(&Value, Payload.GetData() + Offset, sizeof(int64));
	}
	return Value;
}

float UFL_Serialization::DeserializeFloat(const TArray<uint8>& Payload, const int32 Offset)
{
	if (Payload.Num() < Offset + sizeof(float))
	{
		UE_LOG(LogTemp, Error, TEXT("Buffer too small to deserialize float."));
		return 0.0f;
	}

	// Directly copy the bytes from the payload to the float
	float Value;
	FMemory::Memcpy(&Value, &Payload[Offset], sizeof(float));
	return Value;
}

TArray<uint8> UFL_Serialization::CalculateHMAC(const TArray<uint8>& Payload, const FString& GameToken)
{
	TArray<uint8> HVACResult;

	//Convert String to TArray uint8
	TArray<uint8> Key;
	Key.Append(reinterpret_cast<const uint8*>(TCHAR_TO_UTF8(*GameToken)), GameToken.Len());

	uint8 HMACBuffer[32] = {0};

	unsigned int OutLen = 0;
	HMAC(
		EVP_sha256(),
		Key.GetData(),
		Key.Num(),
		Payload.GetData(),
		Payload.Num(),
		HMACBuffer,
		&OutLen
	);

	HVACResult.Append(HMACBuffer, OutLen);

	return HVACResult;
}

bool UFL_Serialization::AuthenticateHMAC(const TArray<uint8>& ReceivedMessage, const FString& GameToken)
{
	return true; // TODO: Fix server HMAC hash and remove this 


	// if (ReceivedMessage.Num() < 37) // Minimum expected size (Payload + HVAC + Header)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("AuthenticateHVAC: Received message is too small to be valid."));
	// 	return false;
	// }
	
	// // HVAC is the last 32 bytes
	// constexpr int32 HMAC_Size = 32;

	// // The payload starts after the header and ends 32 bytes before the message end
	// const int32 PayloadSize = ReceivedMessage.Num() - HMAC_Size;

	// if (PayloadSize <= 0)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("AuthenticateHVAC: Invalid payload length."));
	// 	return false;
	// }

	// // Extract Payload (ignoring header and HVAC)
	// TArray<uint8> ExtractedPayload;
	// ExtractedPayload.Append(ReceivedMessage.GetData(), PayloadSize);
	
	// // Extract Received HVAC (last 32 bytes)
	// TArray<uint8> ReceivedHMAC;
	// ReceivedHMAC.Append(ReceivedMessage.GetData() + ReceivedMessage.Num() - HMAC_Size, HMAC_Size);
	
	// // Recalculate HVAC
	// const TArray<uint8> CalculatedHVAC = UFL_Serialization::CalculateHMAC(ExtractedPayload, GameToken);
	

	// // Compare both
	// if (ReceivedHMAC == CalculatedHVAC)
	// {
	// 	//UE_LOG(LogUDPService, Log, TEXT("AuthenticateHVAC: HVAC authentication successful."));
	// 	return true;
	// }

	// //UE_LOG(LogUDPService, Error, TEXT("AuthenticateHVAC: HVAC authentication failed."));
	// return false;
}

bool UFL_Serialization::ExtractChunkCoordinates(const TSharedPtr<FJsonObject>& JsonObj, int64& X, int64& Y, int64& Z)
{
	const TSharedPtr<FJsonObject>* Coordinates;

	if (!JsonObj->TryGetObjectField(TEXT("coordinates"), Coordinates))
	{
		UE_LOG(LogTemp, Warning, TEXT("Coordinates not found in updateChunk object"));
		return false;
	}

	FString sX, sY, sZ;
	if (!(*Coordinates)->TryGetStringField(TEXT("x"), sX))
	{
		UE_LOG(LogTemp, Warning, TEXT("X coord not found in coordinates object or failed to extract coord"));
		return false;
	}

	if (!(*Coordinates)->TryGetStringField(TEXT("y"), sY))
	{
		UE_LOG(LogTemp, Warning, TEXT("Y coord not found in coordinates object or failed to extract coord"));
		return false;
	}

	if (!(*Coordinates)->TryGetStringField(TEXT("z"), sZ))
	{
		UE_LOG(LogTemp, Warning, TEXT("Z coord not found in coordinates object or failed to extract coord"));
		return false;
	}

	X = FCString::Atoi64(*sX);
	Y = FCString::Atoi64(*sY);
	Z = FCString::Atoi64(*sZ);

	return true;
}
