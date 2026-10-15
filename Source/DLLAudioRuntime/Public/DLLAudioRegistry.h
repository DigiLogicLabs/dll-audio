// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UDLLAudioDefinition;

/**
 * Plain C++ tag -> definition lookup. No UObject or world dependency so it can be unit tested in isolation.
 * Owned by UDLLAudioSubsystem, which keeps the definitions alive.
 */
class DLLAUDIORUNTIME_API FDLLAudioRegistry
{
public:
	void Register(const UDLLAudioDefinition* Definition);
	void Unregister(const UDLLAudioDefinition* Definition);
	void Reset();

	/** Most specific match for Tag + Context, walking up parent tags. Null when nothing matches. */
	const UDLLAudioDefinition* Find(const FGameplayTag& EventTag, const FGameplayTagContainer& Context) const;

	int32 Num() const;

private:
	const UDLLAudioDefinition* FindExact(const FGameplayTag& EventTag, const FGameplayTagContainer& Context) const;

	TMap<FGameplayTag, TArray<TWeakObjectPtr<const UDLLAudioDefinition>>> ByTag;
};

/**
 * Drops repeats of (EventTag, DedupKey) inside a time window. Plain C++ for testability.
 * Bounded: entries older than the window are pruned on every query.
 */
class DLLAUDIORUNTIME_API FDLLAudioDedupFilter
{
public:
	/** True when this occurrence should play; records it. Key 0 always passes. */
	bool Admit(const FGameplayTag& EventTag, int32 DedupKey, double Now, double WindowSeconds);

	int32 NumTracked() const { return Recent.Num(); }

private:
	TMap<uint64, double> Recent;
	double LastPruneTime = -1.0;
};
