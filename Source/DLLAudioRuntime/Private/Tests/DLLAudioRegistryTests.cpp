// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DLLAudioDefinition.h"
#include "DLLAudioRegistry.h"
#include "DLLAudioTags.h"
#include "UObject/Package.h"

namespace DLLAudioTests
{
	static UDLLAudioDefinition* MakeDefinition(const FGameplayTag& EventTag, const FGameplayTagContainer& Required)
	{
		UDLLAudioDefinition* Definition = NewObject<UDLLAudioDefinition>(GetTransientPackage());
		Definition->EventTag = EventTag;
		Definition->RequiredContextTags = Required;
		Definition->AddToRoot(); // test-local lifetime; removed at the end of each test
		return Definition;
	}
}

// Uses only the plugin's own native root tags so the test runs in any project.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioRegistrySpecificityTest, "DLLAudio.Registry.Specificity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioRegistrySpecificityTest::RunTest(const FString& Parameters)
{
	using namespace DLLAudioTests;

	const FGameplayTag EventRoot = DLLAudioTags::Event;
	const FGameplayTag StateRoot = DLLAudioTags::State;
	const FGameplayTag MixRoot = DLLAudioTags::Mix;

	FGameplayTagContainer RequireState;
	RequireState.AddTag(StateRoot);
	FGameplayTagContainer RequireStateAndMix = RequireState;
	RequireStateAndMix.AddTag(MixRoot);

	UDLLAudioDefinition* Generic = MakeDefinition(EventRoot, FGameplayTagContainer());
	UDLLAudioDefinition* OneTag = MakeDefinition(EventRoot, RequireState);
	UDLLAudioDefinition* TwoTags = MakeDefinition(EventRoot, RequireStateAndMix);

	FDLLAudioRegistry Registry;
	Registry.Register(Generic);
	Registry.Register(OneTag);
	Registry.Register(TwoTags);
	TestEqual(TEXT("Three definitions registered"), Registry.Num(), 3);

	TestTrue(TEXT("Empty context -> generic"), Registry.Find(EventRoot, FGameplayTagContainer()) == Generic);
	TestTrue(TEXT("One matching tag -> one-tag variant"), Registry.Find(EventRoot, RequireState) == OneTag);
	TestTrue(TEXT("Both tags -> most specific"), Registry.Find(EventRoot, RequireStateAndMix) == TwoTags);
	TestTrue(TEXT("Unregistered tag with no parent -> null"), Registry.Find(StateRoot, FGameplayTagContainer()) == nullptr);

	Registry.Unregister(TwoTags);
	TestTrue(TEXT("After unregister, falls back to next most specific"), Registry.Find(EventRoot, RequireStateAndMix) == OneTag);

	Generic->RemoveFromRoot();
	OneTag->RemoveFromRoot();
	TwoTags->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDLLAudioDedupTest, "DLLAudio.Registry.DedupWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDLLAudioDedupTest::RunTest(const FString& Parameters)
{
	const FGameplayTag Tag = DLLAudioTags::Event;
	FDLLAudioDedupFilter Filter;

	TestTrue(TEXT("Key 0 always admitted"), Filter.Admit(Tag, 0, 0.0, 0.5));
	TestTrue(TEXT("Key 0 always admitted (repeat)"), Filter.Admit(Tag, 0, 0.0, 0.5));
	TestEqual(TEXT("Key 0 is never tracked"), Filter.NumTracked(), 0);

	TestTrue(TEXT("First keyed event admitted"), Filter.Admit(Tag, 42, 1.0, 0.5));
	TestFalse(TEXT("Repeat inside window dropped"), Filter.Admit(Tag, 42, 1.2, 0.5));
	TestTrue(TEXT("Different key admitted"), Filter.Admit(Tag, 43, 1.2, 0.5));
	TestTrue(TEXT("Same key on another tag admitted"), Filter.Admit(DLLAudioTags::State, 42, 1.2, 0.5));
	TestTrue(TEXT("Repeat after window admitted"), Filter.Admit(Tag, 42, 2.0, 0.5));

	// Prune keeps the map bounded.
	Filter.Admit(Tag, 99, 10.0, 0.5);
	TestEqual(TEXT("Old entries pruned"), Filter.NumTracked(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
