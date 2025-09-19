// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class CKNetwork : ModuleRules
{
	public CKNetwork(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine","InputCore", "AudioCapture", "AudioCaptureCore", "Networking","AudioMixer", "Voice", "OpenSSL", "libOpus", "Sockets", "Networking", "HTTP", "Json",
			"SignalProcessing", "UELibSampleRate", "Skelot", "Json", "JsonUtilities", "UMG"
		});
		

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CKTypes",
			"CKGameObjects"
		});

		bUseUnity = false;
		
	
	}
}
