// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DLLAudioBurst.h"
#include "DLLAudioTypes.h"
#include "Components/AudioComponent.h"
#include "UObject/Package.h"

// Burst merging (FDLLAudioBurstMerge). The invariant worth protecting: an occurrence merges ONLY into a live voice,
// so merging can never make the world quieter than one voice per burst.

namespace DLLAudioBurstTests
{
	static UAudioComponent* MakeVoice()
	{
		// Never played: the tracker only needs a component whose weak pointer stays valid until it "finishes".
		return NewObject<UAudioComponent>(GetTransientPackage());
	}

	static FDLLAudioBurstMerge MakeSettings(int32 MaxVoices)
	{
		FDLLAudioBurstMerge Settings;
		Settings.bEnabled = true;
		Settings.WindowSeconds = 0.15f;
		Settings.CellSize = 1000.0f;
		Settings.MaxVoicesPerBurst = MaxVoices;
		return Settings;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioBurstMergeWindowTest, "DLLAudio.Burst.MergeWindowAndCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioBurstMergeWindowTest::RunTest(const FString& Parameters)
{
	using namespace DLLAudioBurstTests;

	int32 DefinitionA = 0;
	int32 DefinitionB = 0;
	const FDLLAudioBurstMerge Settings = MakeSettings(1);
	FDLLAudioBurstTracker Tracker;
	FDLLAudioBurstTracker::FMergeResult Merge;

	TestFalse(TEXT("No burst open -> render"), Tracker.TryMerge(&DefinitionA, FVector(100, 100, 0), Settings, 0.0, Merge));

	UAudioComponent* Voice = MakeVoice();
	Tracker.AddVoices(&DefinitionA, FVector(100, 100, 0), Settings, 0.0, { Voice }, { 0.8f });
	TestEqual(TEXT("One burst open"), Tracker.NumBursts(), 1);

	if (TestTrue(TEXT("Same definition, same cell, inside window -> merge"), Tracker.TryMerge(&DefinitionA, FVector(900, 200, 50), Settings, 0.05, Merge)))
	{
		TestEqual(TEXT("First merge counted"), Merge.MergedCount, 1);
		TestEqual(TEXT("BurstCount = voice + merged"), Merge.BurstCount, 2);
		TestEqual(TEXT("Merge returns the live voice"), Merge.LiveVoices.Num(), 1);
		TestEqual(TEXT("Voice keeps its own base volume"), Merge.LiveVoices.Num() == 1 ? Merge.LiveVoices[0].BaseVolume : 0.0f, 0.8f);
		TestEqual(TEXT("Origin is where the first voice played"), Merge.Origin, FVector(100, 100, 0));
	}
	TestTrue(TEXT("Second merge"), Tracker.TryMerge(&DefinitionA, FVector(500, 500, 0), Settings, 0.10, Merge));
	TestEqual(TEXT("Counts accumulate"), Merge.BurstCount, 3);

	TestFalse(TEXT("Neighbouring cell -> render"), Tracker.TryMerge(&DefinitionA, FVector(1100, 100, 0), Settings, 0.10, Merge));
	TestFalse(TEXT("Other definition -> render"), Tracker.TryMerge(&DefinitionB, FVector(100, 100, 0), Settings, 0.10, Merge));
	TestFalse(TEXT("Negative coordinates are their own cell"), Tracker.TryMerge(&DefinitionA, FVector(-100, 100, 0), Settings, 0.10, Merge));

	TestFalse(TEXT("After the window -> render"), Tracker.TryMerge(&DefinitionA, FVector(100, 100, 0), Settings, 0.20, Merge));
	TestEqual(TEXT("Expired burst pruned"), Tracker.NumBursts(), 0);

	Tracker.AddVoices(&DefinitionA, FVector(100, 100, 0), Settings, 0.30, TArray<UAudioComponent*>(), TArray<float>());
	TestEqual(TEXT("Nothing rendered -> no burst"), Tracker.NumBursts(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioBurstNeverSilencesTest, "DLLAudio.Burst.NeverMergesIntoFinishedVoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioBurstNeverSilencesTest::RunTest(const FString& Parameters)
{
	using namespace DLLAudioBurstTests;

	int32 Definition = 0;
	const FDLLAudioBurstMerge Settings = MakeSettings(1);
	FDLLAudioBurstTracker Tracker;
	FDLLAudioBurstTracker::FMergeResult Merge;

	UAudioComponent* Short = MakeVoice();
	Tracker.AddVoices(&Definition, FVector::ZeroVector, Settings, 0.0, { Short }, { 1.0f });
	TestTrue(TEXT("Live voice -> merge"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.02, Merge));

	// The sound finished inside the window: its auto-destroyed component invalidates the weak pointer.
	Short->MarkAsGarbage();
	TestFalse(TEXT("Finished voice -> render instead of merging into silence"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.04, Merge));

	UAudioComponent* Fresh = MakeVoice();
	Tracker.AddVoices(&Definition, FVector::ZeroVector, Settings, 0.04, { Fresh }, { 1.0f });
	if (TestTrue(TEXT("New voice starts a new burst"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.05, Merge)))
	{
		TestEqual(TEXT("New burst counts from scratch"), Merge.BurstCount, 2);
	}
	TestTrue(TEXT("New burst has its own window"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.18, Merge));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioBurstVariantTest, "DLLAudio.Burst.DifferentSoundsNeverMerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioBurstVariantTest::RunTest(const FString& Parameters)
{
	using namespace DLLAudioBurstTests;

	// One fallback definition carrying two different caller sounds (a coin and an item pickup) in the same place.
	int32 Definition = 0;
	int32 SoundA = 0;
	int32 SoundB = 0;
	const FDLLAudioBurstMerge Settings = MakeSettings(1);
	FDLLAudioBurstTracker Tracker;
	FDLLAudioBurstTracker::FMergeResult Merge;

	Tracker.AddVoices(&Definition, FVector::ZeroVector, Settings, 0.0, { MakeVoice() }, { 1.0f }, &SoundA);
	TestTrue(TEXT("Same sound -> merge"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.01, Merge, &SoundA));
	TestFalse(TEXT("Different sound, same definition and cell -> render its own voice"),
		Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.01, Merge, &SoundB));
	TestFalse(TEXT("No variant is its own burst too"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.01, Merge));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioBurstVoicesAndVolumeTest, "DLLAudio.Burst.MaxVoicesAndVolumeScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioBurstVoicesAndVolumeTest::RunTest(const FString& Parameters)
{
	using namespace DLLAudioBurstTests;

	int32 Definition = 0;
	const FDLLAudioBurstMerge Settings = MakeSettings(2);
	FDLLAudioBurstTracker Tracker;
	FDLLAudioBurstTracker::FMergeResult Merge;

	Tracker.AddVoices(&Definition, FVector::ZeroVector, Settings, 0.0, { MakeVoice() }, { 1.0f });
	TestFalse(TEXT("Room for a second voice -> render"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.01, Merge));
	Tracker.AddVoices(&Definition, FVector::ZeroVector, Settings, 0.01, { MakeVoice(), MakeVoice() }, { 1.0f, 0.5f });
	if (TestTrue(TEXT("Voices full -> merge"), Tracker.TryMerge(&Definition, FVector::ZeroVector, Settings, 0.02, Merge)))
	{
		TestEqual(TEXT("BurstCount counts occurrences, not layers"), Merge.BurstCount, 3);
		TestEqual(TEXT("Every layer of every voice is updated"), Merge.LiveVoices.Num(), 3);
	}

	FDLLAudioBurstMerge Growth = Settings;
	Growth.VolumeGrowthPerMergedEvent = 0.25f;
	Growth.MaxVolumeScale = 1.5f;
	TestEqual(TEXT("No merges -> unchanged"), FDLLAudioBurstTracker::VolumeScaleFor(Growth, 0), 1.0f);
	TestEqual(TEXT("One merge -> +25%"), FDLLAudioBurstTracker::VolumeScaleFor(Growth, 1), 1.25f);
	TestEqual(TEXT("Capped at MaxVolumeScale"), FDLLAudioBurstTracker::VolumeScaleFor(Growth, 10), 1.5f);

	Growth.VolumeGrowthPerMergedEvent = 0.0f;
	TestEqual(TEXT("Zero growth -> volume never changes"), FDLLAudioBurstTracker::VolumeScaleFor(Growth, 10), 1.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
