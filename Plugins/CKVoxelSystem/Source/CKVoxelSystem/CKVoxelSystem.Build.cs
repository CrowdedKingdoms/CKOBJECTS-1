// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CKVoxelSystem : ModuleRules
{
	public CKVoxelSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "EnhancedInput",
				"EnhancedInput", "Sockets", "Networking", "HTTP", 
				"ProceduralMeshComponent", "OpenSSL", "libOpus", "AudioCapture", "AudioCaptureCore", "AudioMixer", "Voice",
				"SignalProcessing", "UELibSampleRate", "Json", "JsonUtilities", "UMG"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"CKSharedTypes",
				"Skelot"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
