// Fill out your copyright notice in the Description page of Project Settings.
//, "CKVoxelSystem", "CKNetwork", "CKPlayer", "CKTypes"
using UnrealBuildTool;
using System.Collections.Generic;

public class CKOBJECTSEditorTarget : TargetRules
{
	public CKOBJECTSEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "CKOBJECTS" } );
	}
}
