#pragma once

#include "CoreMinimal.h"
#include "Shared/Types/Enums/Character/EActionType.h"
#include "Shared/Types/Enums/Character/ECharacterType.h"
#include "Shared/Types/Enums/Character/EGait.h"
#include "GameFramework/Character.h"
#include "FActorState.generated.h"

USTRUCT(BlueprintType)
struct FActorState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="ActorState")
	uint8 Version;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	FVector Velocity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bIsNPC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bIsFlying;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bIsJumping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bIsFalling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bJustLanded;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bTraversal;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	float ObstacleDepth;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	float ObstacleHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	EActionType ActionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	bool bCrouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
	EGait Gait;
	    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActorState")
    ECharacterType CharacterType;

	
    
	FActorState()
		: Version(1),
		  Position(FVector::ZeroVector),
		  Rotation(FRotator(0, 0, 0)),
		  Velocity(FVector::ZeroVector),
		  bIsNPC(true),
		  bIsFlying(false),
		  bIsJumping(false), bIsFalling(false), bJustLanded(false),
		  bTraversal(false),
		  ObstacleDepth(0),
		  ObstacleHeight(0),
		  ActionType(EActionType::None),
		  bCrouch(false),
		  Gait(EGait::Run),
		  CharacterType(ECharacterType::SandboxCharacter)
	{
	}

	static TArray<uint8> SerializeActorState(const FActorState& ActorState)
	{
        TArray<uint8> ByteArray;
        ByteArray.SetNumUninitialized(sizeof(ActorState));
        FMemory::Memcpy(ByteArray.GetData(), &ActorState, sizeof(ActorState));
        return ByteArray;
    }

	static FActorState DeserializeActorState(const TArray<uint8>& Payload, const int32 Offset)
	{
        FActorState ActorState;
        if(Payload.Num() >= Offset + sizeof(FActorState))
        {
            FMemory::Memcpy(&ActorState, Payload.GetData() + Offset, sizeof(FActorState));
        }
        
        return ActorState;
    }
    
   
    
};
