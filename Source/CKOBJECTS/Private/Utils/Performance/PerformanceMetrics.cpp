// Fill out your copyright notice in the Description page of Project Settings.
#include "Utils/Performance/PerformanceMetrics.h"


// Sets default values
APerformanceMetrics::APerformanceMetrics(): FramesPerSecond(0), CPUUsage(0), TotalMemory(0), UsedMemory(0),
                                            UsedMemoryPercentage(0)
#if PLATFORM_WINDOWS
    , pdhCpuQuery(nullptr), pdhCpuCounter(nullptr), lastCPU(), lastSysCPU(), lastUserCPU(), numProcessors(0), hProcess(nullptr), lastSysKernel(), lastSysUser(), lastSysIdle()
#elif PLATFORM_MAC
    , lastCpuTime(0), lastIdleTime(0), machHost(0)
#endif
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.00f;
}

// Called when the game starts or when spawned
void APerformanceMetrics::BeginPlay()
{
	Super::BeginPlay();
#if PLATFORM_WINDOWS
	// Get the number of processors
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	numProcessors = FPlatformMisc::NumberOfCores();
#elif PLATFORM_MAC
	machHost = mach_host_self();
#endif
}

void APerformanceMetrics::UpdateMetrics()
{
	FramesPerSecond = 1.0f / GetWorld()->GetDeltaSeconds();
	MemoryStats = FPlatformMemory::GetStats();
	TotalMemory = MemoryStats.TotalPhysical/(1024.0f * 1024.0f * 1024.0f);
	UsedMemory = MemoryStats.UsedPhysical/(1024.0f * 1024.0f * 1024.0f);
	UsedMemoryPercentage = (UsedMemory/TotalMemory)*100.0f;
	
}

double APerformanceMetrics::CalculateCPUUsage()
{
#if PLATFORM_WINDOWS
	FILETIME ftSysIdle, ftSysKernel, ftSysUser;
	ULARGE_INTEGER now, sysKernel, sysUser;

	GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser);

	sysKernel.LowPart = ftSysKernel.dwLowDateTime;
	sysKernel.HighPart = ftSysKernel.dwHighDateTime;

	sysUser.LowPart = ftSysUser.dwLowDateTime;
	sysUser.HighPart = ftSysUser.dwHighDateTime;

	now.LowPart = ftSysIdle.dwLowDateTime;
	now.HighPart = ftSysIdle.dwHighDateTime;

	ULARGE_INTEGER sysTime;
	sysTime.QuadPart = (sysKernel.QuadPart + sysUser.QuadPart);

	ULONGLONG deltaTime = (sysTime.QuadPart - lastSysCPU.QuadPart);
	ULONGLONG deltaIdle = (now.QuadPart - lastSysIdle.QuadPart);

	lastSysCPU = sysTime;
	lastSysIdle = now;

	double cpuUsage = (double)(deltaTime - deltaIdle) / deltaTime * 100.0;

	// Normalize for all cores
	cpuUsage = cpuUsage / FPlatformMisc::NumberOfCores();

	return cpuUsage * 100; // Convert to a percentage of total CPU usage
#elif PLATFORM_MAC
	// macOS implementation using mach system calls
	host_cpu_load_info_data_t cpuinfo;
	mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
	
	if (host_statistics(machHost, HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS)
	{
		uint64_t totalTime = cpuinfo.cpu_ticks[CPU_STATE_USER] + cpuinfo.cpu_ticks[CPU_STATE_NICE] + 
		                     cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] + cpuinfo.cpu_ticks[CPU_STATE_IDLE];
		uint64_t idleTime = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
		
		if (lastCpuTime > 0)
		{
			uint64_t totalDelta = totalTime - lastCpuTime;
			uint64_t idleDelta = idleTime - lastIdleTime;
			
			if (totalDelta > 0)
			{
				double cpuUsage = (double)(totalDelta - idleDelta) / totalDelta * 100.0;
				lastCpuTime = totalTime;
				lastIdleTime = idleTime;
				return cpuUsage;
			}
		}
		
		lastCpuTime = totalTime;
		lastIdleTime = idleTime;
	}
	
	return 0.0;
#else
	// Fallback for other platforms
	return 0.0;
#endif
}

// Called every frame
void APerformanceMetrics::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateMetrics();
	CPUUsage = CalculateCPUUsage();
	//UE_LOG(LogTemp, Log, TEXT("FPS: %f"), FramesPerSecond);
	//UE_LOG(LogTemp, Log, TEXT("TotalMem:%f, UsedMem:%f, UsedMemPerc:%f"), TotalMemory, UsedMemory, UsedMemoryPercentage);
	//UE_LOG(LogTemp, Log, TEXT("CPU usage: %f"), CPUUsage);
	//FString Msg = FString::Printf(TEXT("CPU usage: %f"), CPUUsage);
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Msg);
}

