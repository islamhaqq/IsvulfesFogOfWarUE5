// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class FOWForkTarget : TargetRules
{
	public FOWForkTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ShadowVariableWarningLevel = WarningLevel.Error;
		bLegacyParentIncludePaths = false;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.AddRange( new string[] { "FOWFork" } );
	}
}
