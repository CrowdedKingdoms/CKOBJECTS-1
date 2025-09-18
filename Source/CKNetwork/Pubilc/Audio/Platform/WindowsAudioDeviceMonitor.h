// WindowsAudioDeviceMonitor.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/WeakObjectPtr.h"

#if PLATFORM_WINDOWS
// Windows specific headers
#include "Windows/AllowWindowsPlatformTypes.h"
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
using namespace Microsoft::WRL;
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "WindowsAudioDeviceMonitor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioDeviceConnected, const FString&, DeviceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioDeviceDisconnected, const FString&, DeviceId);

#if PLATFORM_WINDOWS
// Forward declaration for Windows notification client
class FWindowsAudioDeviceNotificationClient;
#endif

UCLASS(ClassGroup=(Audio))
class CROWDEDKINGDOMS_API AWindowsAudioDeviceMonitor : public AActor
{
    GENERATED_BODY()

public:
    AWindowsAudioDeviceMonitor();
    virtual ~AWindowsAudioDeviceMonitor() override;

    // Called when the game starts
    virtual void BeginPlay() override;
    
    // Called when the game ends
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Initialize the monitor
    UFUNCTION(BlueprintCallable, Category = "Audio|Device")
    void InitializeAudioDeviceMonitoring();

    // Shutdown the monitor
    UFUNCTION(BlueprintCallable, Category = "Audio|Device")
    void ShutdownAudioDeviceMonitoring();

    // Get list of available audio devices
    UFUNCTION(BlueprintCallable, Category = "Audio|Device")
    TArray<FString> GetAvailableAudioDevices(bool bOutputDevices = true);

    // Delegate fired when an audio device is connected
    UPROPERTY(BlueprintAssignable, Category = "Audio|Device")
    FOnAudioDeviceConnected OnAudioDeviceConnected;

    // Delegate fired when an audio device is disconnected
    UPROPERTY(BlueprintAssignable, Category = "Audio|Device")
    FOnAudioDeviceDisconnected OnAudioDeviceDisconnected;

    // Internal callback when a device is connected
    void HandleDeviceConnected(const FString& DeviceId);
    
    // Internal callback when a device is disconnected
    void HandleDeviceDisconnected(const FString& DeviceId);
private:
#if PLATFORM_WINDOWS
    // Pointer to the notification client
    TSharedPtr<FWindowsAudioDeviceNotificationClient> NotificationClient;
    
    // COM Pointers for the audio device enumerator
    ComPtr<IMMDeviceEnumerator> DeviceEnumerator;
#endif
    
    
    
    // Flag to track if monitoring is active
    bool bIsMonitoringActive;
};