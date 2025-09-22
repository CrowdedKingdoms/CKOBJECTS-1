// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

// Platform-specific includes
#if PLATFORM_WINDOWS
    #include "Windows/AllowWindowsPlatformTypes.h"
    #include "Windows/WindowsPlatformTime.h"
    #include <pdh.h>
    #include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_MAC
    #include <mach/mach.h>
    #include <sys/sysctl.h>
#endif

#include "PerformanceMetrics.generated.h"

UCLASS()
class  APerformanceMetrics : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APerformanceMetrics();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Performance Metrics")
	float GetCPUUsage(){return CPUUsage;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Performance Metrics")
	float GetFramesPerSecond(){return FramesPerSecond;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Performance Metrics")
	float GetTotalMemory(){return TotalMemory;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Performance Metrics")
	float GetUsedMemory(){return UsedMemory;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Performance Metrics")
	float GetUsedMemoryPercentage(){return UsedMemoryPercentage;}
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void UpdateMetrics();
	double CalculateCPUUsage();
	
private:

	FPlatformMemoryStats MemoryStats;
	
	float FramesPerSecond;
	double CPUUsage;
	float TotalMemory;
	float UsedMemory;
	float UsedMemoryPercentage;
	
#if PLATFORM_WINDOWS
	// PDH-based system-wide CPU usage
	PDH_HQUERY pdhCpuQuery;
	PDH_HCOUNTER pdhCpuCounter;

	// Windows API method
	ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
	int numProcessors;
	HANDLE hProcess;

	ULARGE_INTEGER lastSysKernel;
	ULARGE_INTEGER lastSysUser;
	ULARGE_INTEGER lastSysIdle;
#elif PLATFORM_MAC
	// macOS-specific CPU monitoring variables
	uint64_t lastCpuTime;
	uint64_t lastIdleTime;
	mach_port_t machHost;
#endif
    
};
