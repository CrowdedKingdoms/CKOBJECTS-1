// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class CKVoxelSystem : ModuleRules
{
	public CKVoxelSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", 
			"SignalProcessing", "UELibSampleRate", "Skelot", "Json", "JsonUtilities", "UMG",
			
			"Sockets", "Networking", "HTTP",
			"ProceduralMeshComponent", "OpenSSL", "libOpus", "AudioCapture", "AudioCaptureCore", "AudioMixer", "Voice"
		});
		

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CKTypes", "CKSharedTypes"
		});
		
		bUseUnity = false;
		
	}
}
