// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/Services/Communication/TextChatServiceSubsystem.h"
#include "FunctionLibraries/Network/FL_Serialization.h"
#include "Network/Infrastructure/NetworkMessageParser.h"
#include "Network/Services/Core/UDPSubsystem.h"
#include "Shared/Types/Core/GameSessionSubsystem.h"

void UTextChatServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTextChatServiceSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTextChatServiceSubsystem::PostSubsystemInit()
{
	UDPSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UUDPSubsystem>();

	if (!UDPSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Text Chat Service: Subsystem(s) reference is invalid."));
	}
}


void UTextChatServiceSubsystem::SendTextChatMessage(const FTextMessage& TextMessageToSend)
{
	if (TextMessageToSend.Message.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempted to send empty message"));
		return;
	}

	TArray<uint8> Payload;

	const int64 MapId = GetGameInstance()->GetSubsystem<UGameSessionSubsystem>()->GetMapID();
	const FInt64Vector ChunkCoord = GetGameInstance()->GetSubsystem<UGameSessionSubsystem>()->GetPlayerCurrentChunkCoordinates();
	const FString UUID = GetGameInstance()->GetSubsystem<UGameSessionSubsystem>()->GetUUID();

	Payload.Append(reinterpret_cast<const uint8*>(&MapId), sizeof(int64));
	Payload.Append(reinterpret_cast<const uint8*>(&ChunkCoord.X), sizeof(int64));
	Payload.Append(reinterpret_cast<const uint8*>(&ChunkCoord.Y), sizeof(int64));
	Payload.Append(reinterpret_cast<const uint8*>(&ChunkCoord.Z), sizeof(int64));

	// Convert FString to UTF-8 bytes
	const FTCHARToUTF8 UTF8String(*UUID);

	// Add the UTF-8 bytes to the payload
	const TArray UUIDBytes(reinterpret_cast<const uint8*>(UTF8String.Get()), UTF8String.Length());
	Payload.Append(UUIDBytes);

	
	// Write UserID
	const int64 UserID = TextMessageToSend.UserID;
	Payload.Append(reinterpret_cast<const uint8*>(&UserID), sizeof(int64));
    
	// Write Username
	const FTCHARToUTF8 ConvertedUsername(*TextMessageToSend.Username);
	const int32 UsernameLen = ConvertedUsername.Length();  // Get actual length
	Payload.Append(reinterpret_cast<const uint8*>(&UsernameLen), sizeof(int32));
	Payload.Append(reinterpret_cast<const uint8*>(ConvertedUsername.Get()), UsernameLen);
    
	// Write Message
	const FTCHARToUTF8 ConvertedMessage(*TextMessageToSend.Message);
	const int32 MessageLen = ConvertedMessage.Length();  // Get actual length
	Payload.Append(reinterpret_cast<const uint8*>(&MessageLen), sizeof(int32));
	Payload.Append(reinterpret_cast<const uint8*>(ConvertedMessage.Get()), MessageLen);

	if (UDPSubsystem)
	{
		if (!UDPSubsystem->QueueUDPMessage(EMessageType::CLIENT_TEXT_PACKET, Payload))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to send text message"));
		} else {
			UE_LOG(LogTemp, Log, TEXT("Text message sent"));
		}
	}
	
}

void UTextChatServiceSubsystem::HandleIncomingTextChatMessage(const TArray<uint8>& Payload) const
{
	if (Payload.Num() < sizeof(int64))
    {
        UE_LOG(LogTemp, Warning, TEXT("Received invalid payload size"));
        return;
    }

    int32 Offset = 0;
	Offset += sizeof(int64); //skip mapID
	Offset += sizeof(int64); //skip ChunkX
	Offset += sizeof(int64); //skip ChunkY
	Offset += sizeof(int64); //skip ChunkZ

	const FString ReceivedUUID = UFL_Serialization::DeserializeString(Payload, Offset, 32);

	Offset += 32; //skip UUID
	
	if (ReceivedUUID.Compare(GetGameInstance()->GetSubsystem<UGameSessionSubsystem>()->GetUUID()))
	{
		return;
	}
	
    FTextMessage ReceivedMessage;

    // Read UserID
    FMemory::Memcpy(&ReceivedMessage.UserID, Payload.GetData() + Offset, sizeof(int64));
    Offset += sizeof(int64);

    // Read Username
    if (Offset + sizeof(int32) <= Payload.Num())
    {
        int32 UsernameLen;
        FMemory::Memcpy(&UsernameLen, Payload.GetData() + Offset, sizeof(int32));
        Offset += sizeof(int32);

        if (UsernameLen > 0 && Offset + UsernameLen <= Payload.Num())
        {
            // Create a buffer with space for null terminator
            TArray<ANSICHAR> UsernameBuffer;
            UsernameBuffer.SetNum(UsernameLen + 1);  // +1 for null terminator
            FMemory::Memcpy(UsernameBuffer.GetData(), Payload.GetData() + Offset, UsernameLen);
            UsernameBuffer[UsernameLen] = '\0';  // Add null terminator after the full string
            
            ReceivedMessage.Username = FString(UTF8_TO_TCHAR(UsernameBuffer.GetData()));
            Offset += UsernameLen;
        }
    }

    // Read Message
    if (Offset + sizeof(int32) <= Payload.Num())
    {
        int32 MessageLen;
        FMemory::Memcpy(&MessageLen, Payload.GetData() + Offset, sizeof(int32));
        Offset += sizeof(int32);

        if (MessageLen > 0 && Offset + MessageLen <= Payload.Num())
        {
            // Create a buffer with space for null terminator
            TArray<ANSICHAR> MessageBuffer;
            MessageBuffer.SetNum(MessageLen + 1);  // +1 for null terminator
            FMemory::Memcpy(MessageBuffer.GetData(), Payload.GetData() + Offset, MessageLen);
            MessageBuffer[MessageLen] = '\0';  // Add null terminator after the full string
            
            ReceivedMessage.Message = FString(UTF8_TO_TCHAR(MessageBuffer.GetData()));
        }
    }

    if (!ReceivedMessage.Message.IsEmpty())
    {
        const FTextMessage CapturedMessage = ReceivedMessage;
        
        FFunctionGraphTask::CreateAndDispatchWhenReady([this, CapturedMessage]()
        {
            if (IsValid(this))
            {
                UE_LOG(LogTemp, Log, TEXT("Received message - User: %s, Message: %s"), 
                    *CapturedMessage.Username, 
                    *CapturedMessage.Message);

                if (CapturedMessage.UserID == GetGameInstance()->GetSubsystem<UGameSessionSubsystem>()->GetUserID())
                {
                	UE_LOG(LogTemp, Warning, TEXT("Received message from self. Returning early."));
	                return;
                }
                OnTextMessageReceived.Broadcast(CapturedMessage);
            }
        }, TStatId(), nullptr, ENamedThreads::GameThread);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Received empty message"));
    }
}
