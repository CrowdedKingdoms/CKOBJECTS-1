#pragma once

#include <chrono>

#include "CoreMinimal.h"
#include "Shared/Types/Enums/Network/MessageType.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "UObject/Object.h"
#include "UDPSubsystem.generated.h"

class FUDPListenerRunnable;
class UMessageBufferPoolSubsystem;
class UGameSessionSubsystem;


DECLARE_LOG_CATEGORY_EXTERN(LogUDPService, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDPEndpointSet, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUDPTimeout);

USTRUCT(BlueprintType)
struct FUDPNetworkStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 BytesSent = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 BytesReceived = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MessagesSent = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MessagesReceived = 0;
};


UCLASS(Blueprintable, BlueprintType)
class   UUDPSubsystem : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Network|UDP")
	virtual void PostSubsystemInit() override;

	
	/** Sends a UDP datagram to the specified address */
	bool QueueUDPMessage(EMessageType MessageType, const TArray<uint8>& Data) const;

	bool SendUDPMessage(TArray<uint8>& Message);
	
	/** Handle receiving the UDP_ADDRESS_NOTIFICATION_2 and setup UDP socket */
	void HandleUDPAddressNotification(const TSharedPtr<FJsonObject>& Payload);
	
	UFUNCTION(BlueprintCallable, Category = "Network|Statistics")
    FUDPNetworkStats GetUDPNetworkStats() const;

	UPROPERTY(BlueprintAssignable, Category = "Network|Statistics")
	FOnUDPEndpointSet OnUDPEndpointSet;

	UPROPERTY(BlueprintAssignable, Category = "Network|Statistics")
	FOnUDPTimeout OnUDPTimeout;

	// Functions to control timeout
	UFUNCTION(BlueprintCallable, Category = "UDP Timeout")
	void StartTimeoutMonitoring(float ThresholdSeconds = 30.0f);
    
	UFUNCTION(BlueprintCallable, Category = "UDP Timeout")
	void StopTimeoutMonitoring();
    
	UFUNCTION(BlueprintCallable, Category = "UDP Timeout")
	float GetTimeSinceLastMessage() const;

	void HandleUDPMessage(const uint8* Data, int32 Size, const FInternetAddr& Addr);

	

private:
	
	/** Checks if the UDP socket is ready */
	bool IsUDPReady() const { return bUDPReady; }
	
	/** Creates the UDP Send Socket and binds it to the server address and port */
	bool InitializeUDPSocket(const FString& IPAddress, const int32 Port);

	/** Creates the UDP Send Socket (IPv4) and binds it to the server address and port */
	bool InitializeV4UDPSocket(const FString& IPAddress, const int32 Port);

	/** Sends a UDP datagram over IPv6 */
	bool SendUDPv6(const TArray<uint8>& Message) const;

	/** Sends a UDP datagram over IPv4 */
	bool SendUDPv4(const TArray<uint8>& Message) const;
	
	// Starts Listening on UDP port
	void StartUDPListener();

	/** Closes the UDP socket */
	void StopUDPListener();

	// Starts Listening on UDP IPv4 port
	void StartUDPv4Listener();

	/** Closes the UDP IPv4 socket */
	void StopUDPv4Listener();

	// Switch from IPv6 to IPv4
	void SwitchToIPv4();

	void CleanupSockets();
	
	/** Whether the UDP socket is ready */
	bool bUDPReady = false;
	FThreadSafeBool bUDPListen = false;
	FThreadSafeBool bUDPv4Listen = false;

	// Sockets and related
	FSocket* UDPSocket;
	FSocket* UDPSocketV4;
	
	// Send and receive UDP datagrams over IPv4 socket connection instead of IPv6
	mutable bool bUseIPv4 = false;

	// For Network Stats
	UPROPERTY()
	mutable int32 BytesSent;

	UPROPERTY()
	int32 BytesReceived;

	UPROPERTY()
	mutable int32 MessagesSent;

	UPROPERTY()
	int32 MessagesReceived;

	FTimerHandle UDPStatsTimerHandle;

	FThreadSafeBool bTimeoutEnabled = false;
	std::atomic<float> TimeoutThresholdSeconds = 5.0f;
	std::atomic<std::chrono::steady_clock::time_point> LastMessageTime;


	FTimerHandle TimeoutCheckTimerHandle;

	void CheckForTimeout();
	// Call this when UDP message is received
	void StopAllUDPOperations();


protected:
	void UpdateUDPNetworkStats();
	
	UPROPERTY()
	UMessageBufferPoolSubsystem* BufferPoolSubsystem;

	UPROPERTY()
	UGameSessionSubsystem* GameSessionSubsystem;

	void OnMessageReceived(); 
	
	FRunnableThread* ListenerThread = nullptr;
	FUDPListenerRunnable* ListenerRunnable = nullptr;
};
