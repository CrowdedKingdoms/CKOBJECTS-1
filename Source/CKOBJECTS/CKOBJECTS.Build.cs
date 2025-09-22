// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class CKOBJECTS : ModuleRules
{
	public CKOBJECTS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Sockets", "Networking", "HTTP",
			"ProceduralMeshComponent", "OpenSSL", "libOpus", "AudioCapture", "AudioCaptureCore", "AudioMixer", "Voice",
			"SignalProcessing", "UELibSampleRate", "Skelot", "Json", "JsonUtilities", "UMG", "CKSharedTypes"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{

		});
		
		bUseUnity = false;
		
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.Add("Pdh.lib");
        }

        // Assimp dependencies - platform-aware configuration
        string AssimpPath = Path.Combine(ModuleDirectory, "../ThirdParty/Assimp");
        string AssimpIncludePath = Path.Combine(AssimpPath, "Include");
        string AssimpLibPath = Path.Combine(AssimpPath, "Lib");

        PublicIncludePaths.Add(AssimpIncludePath);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(Path.Combine(AssimpLibPath, "assimp-vc143-mt.lib"));
            PublicDelayLoadDLLs.Add("assimp-vc143-mt.dll");
            RuntimeDependencies.Add("$(BinaryOutputDir)/assimp-vc143-mt.dll",
                Path.Combine(AssimpLibPath, "assimp-vc143-mt.dll"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add(Path.Combine(AssimpLibPath, "libassimp.dylib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(Path.Combine(AssimpLibPath, "libassimp.so"));
        }
	}
}
