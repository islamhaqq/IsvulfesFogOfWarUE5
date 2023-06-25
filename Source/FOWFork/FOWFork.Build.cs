// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class FOWFork : ModuleRules
{
	public FOWFork(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ShadowVariableWarningLevel = WarningLevel.Error;
		bLegacyParentIncludePaths = false;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "SimpleFoW" });

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
