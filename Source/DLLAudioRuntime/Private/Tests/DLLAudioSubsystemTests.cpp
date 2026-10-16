// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DLLAudioDefinition.h"
#include "DLLAudioSubsystem.h"
#include "DLLAudioTags.h"
#include "UObject/Package.h"

// Resolution only - no world, no listener, nothing rendered. The subsystem is created outside a world on purpose:
// ResolveAudioEvent touches only the registry and the resolver chain.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioCallerLayersTest, "DLLAudio.Subsystem.CallerLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioCallerLayersTest::RunTest(const FString& Parameters)
{
	UDLLAudioSubsystem* Subsystem = NewObject<UDLLAudioSubsystem>(GetTransientPackage());
	Subsystem->AddToRoot();

	UDLLAudioDefinition* Definition = NewObject<UDLLAudioDefinition>(GetTransientPackage());
	Definition->EventTag = DLLAudioTags::Event;
	Definition->bPreloadSounds = false; // no listener outside a world; keep registration side-effect free
	Definition->AddToRoot();
	Subsystem->RegisterDefinition(Definition);

	FDLLAudioLayer CallerLayer;
	CallerLayer.VolumeMultiplier = 0.5f;

	FDLLAudioEvent Event;
	Event.EventTag = DLLAudioTags::Event;
	Event.CallerLayers.Add(CallerLayer);

	const FDLLAudioResolution WithDefinition = Subsystem->ResolveAudioEvent(Event);
	TestTrue(TEXT("Definition still comes from the registry"), WithDefinition.Definition == Definition);
	TestEqual(TEXT("Caller layers seed ExtraLayers"), WithDefinition.ExtraLayers.Num(), 1);
	if (WithDefinition.ExtraLayers.Num() == 1)
	{
		TestEqual(TEXT("Caller layer settings carried over"), WithDefinition.ExtraLayers[0].VolumeMultiplier, 0.5f);
	}

	// No definition for this tag: caller layers are visible to resolvers but nothing would render.
	Event.EventTag = DLLAudioTags::State;
	const FDLLAudioResolution WithoutDefinition = Subsystem->ResolveAudioEvent(Event);
	TestTrue(TEXT("No definition -> null (caller layers never render alone)"), WithoutDefinition.Definition == nullptr);
	TestEqual(TEXT("Caller layers still visible to resolvers"), WithoutDefinition.ExtraLayers.Num(), 1);

	FDLLAudioEvent Plain;
	Plain.EventTag = DLLAudioTags::Event;
	TestEqual(TEXT("No caller layers -> no extra layers"), Subsystem->ResolveAudioEvent(Plain).ExtraLayers.Num(), 0);

	Subsystem->UnregisterDefinition(Definition);
	Definition->RemoveFromRoot();
	Subsystem->RemoveFromRoot();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
