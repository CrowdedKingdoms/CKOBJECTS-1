// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class CKTypes : ModuleRules
{
	public CKTypes(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",  "CKSharedTypes", "OpenSSL", "libOpus",
			"SignalProcessing", "UELibSampleRate", "Skelot", "Json", "JsonUtilities", "UMG"
		});
		

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
		
		bUseUnity = false;
		
	
	}
}
