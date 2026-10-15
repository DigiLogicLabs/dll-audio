// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "DLLAudioDefinition.h"
#include "DLLAudioResolver.h"
#include "DLLAudioSettings.h"

const FPrimaryAssetType UDLLAudioDefinition::PrimaryAssetType(TEXT("DLLAudioDefinition"));
const FPrimaryAssetType UDLLAudioDefinitionLibrary::PrimaryAssetType(TEXT("DLLAudioDefinitionLibrary"));

FPrimaryAssetId UDLLAudioDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

FPrimaryAssetId UDLLAudioDefinitionLibrary::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

UWorld* UDLLAudioResolver::GetWorld() const
{
	// Resolvers are outered to the subsystem, which is outered to the world.
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}

UDLLAudioSettings::UDLLAudioSettings()
{
	SectionName = TEXT("DLL Audio");
}
