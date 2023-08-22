// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SimpleFoW : ModuleRules
{
	public SimpleFoW(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "CoreUObject", "Engine", "RenderCore", "RHI"
			}
		);
	}
}
