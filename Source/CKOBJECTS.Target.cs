// Fill out your copyright notice in the Description page of Project Settings.
//, "CKVoxelSystem", "CKNetwork", "CKPlayer", "CKTypes"


using UnrealBuildTool;
using System.Collections.Generic;

public class CKOBJECTSTarget : TargetRules
{
	public CKOBJECTSTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "CKOBJECTS","CKVoxelSystem", "CKTypes", "CKGameObjects" } );
	}
}
