// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

using UnrealBuildTool;

public class DLLAudioRuntime : ModuleRules
{
	public DLLAudioRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Core stays engine + GameplayTags only. GAS / Mass / acoustics integrations live in
		// separate optional modules so adopting projects never inherit a dependency they don't use.
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"AudioExtensions",
				"DeveloperSettings"
			}
		);
	}
}
