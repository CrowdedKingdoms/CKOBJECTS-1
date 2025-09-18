#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EMessageType : uint8
{
    BAD_MESSAGE = 0  UMETA(DisplayName = "Bad Message"),
    ACTOR_UPDATE_REQUEST = 1 UMETA(DisplayName = "Actor Update Request"),
    ACTOR_UPDATE_RESPONSE = 2 UMETA(DisplayName = "Actor Update Response"),
    ACTOR_UPDATE_NOTIFICATION = 3 UMETA(DisplayName = "Actor Update Notification"),
    VOXEL_UPDATE_REQUEST = 4 UMETA(DisplayName = "Voxel Update Request"),
    VOXEL_UPDATE_RESPONSE = 5 UMETA(DisplayName = "Voxel Update Response"),
    VOXEL_UPDATE_NOTIFICATION = 6 UMETA(DisplayName = "Voxel Update Notification"),
    CLIENT_AUDIO_PACKET = 7 UMETA(DisplayName = "Client Audio Packet"),
    CLIENT_AUDIO_NOTIFICATION = 8 UMETA(DisplayName = "Client Audio Notification"),
    CLIENT_TEXT_PACKET = 9 UMETA(DisplayName = "Client Text Packet"),
    CLIENT_TEXT_NOTIFICATION = 10 UMETA(DisplayName = "Client Text Notification"),
    CLIENT_EVENT_NOTIFICATION = 11 UMETA(DisplayName = "Client Event Notification"),
    SERVER_EVENT_NOTIFICATION = 12 UMETA(DisplayName = "Server Event Notification")
};


UENUM(BlueprintType)
enum class EErrorCode : uint8
{
    SUCCESS = 0 UMETA(DisplayName = "No Error"),
    UNKNOWN_ERROR = 1 UMETA(DisplayName = "Unknown Error"),
    EMAIL_NOT_FOUND = 2 UMETA(DisplayName = "Email Not Found"),
    BAD_PASSWORD = 3 UMETA(DisplayName = "Bad Password"),
    EMAIL_ALREADY_EXISTS = 4 UMETA(DisplayName = "Email Already Exists"),
    INVALID_TOKEN = 5 UMETA(DisplayName = "Invalid Token"),
    MAP_NOT_FOUND = 6 UMETA(DisplayName = "Map Not Found"),
    UNAUTHORIZED = 7 UMETA(DisplayName = "Unauthorized"),
    MAP_NOT_LOADED = 8 UMETA(DisplayName = "Map Not Loaded"),
    EMAIL_TOO_SHORT = 9 UMETA(DisplayName = "Email Too Short"),
    EMAIL_TOO_LONG = 10 UMETA(DisplayName = "Email Too Long"),
    PASSWORD_TOO_SHORT = 11 UMETA(DisplayName = "Password Too Short"),
    PASSWORD_TOO_LONG = 12 UMETA(DisplayName = "Password Too Long"),
    GAME_TOKEN_WRONG_SIZE = 13 UMETA(DisplayName = "Game Token Wrong Size"),
    NAME_TOO_LONG = 14 UMETA(DisplayName = "Name Too Long"),
    INVALID_REQUEST = 15 UMETA(DisplayName = "Invalid Request"),
    EMAIL_INVALID = 16 UMETA(DisplayName = "Email Invalid"),
    INVALID_TOKEN_LENGTH = 17 UMETA(DisplayName = "Invalid Token Length"),
    INVALID_MAP_ID = 18 UMETA(DisplayName = "Invalid Map ID"),
    CHUNK_NOT_FOUND = 19 UMETA(DisplayName = "Chunk Not Found"),
    USER_NOT_AUTHENTICATED = 20 UMETA(DisplayName = "User Not Authenticated")

};

UENUM(BlueprintType)
enum class EActorType : uint8
{
    NONE = 0  UMETA(DisplayName = "None"),
    NPC_1 = 1 UMETA(DisplayName = "Random Walker NPC"),
};