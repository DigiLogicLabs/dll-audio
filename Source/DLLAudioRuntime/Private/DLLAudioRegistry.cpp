// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "DLLAudioRegistry.h"
#include "DLLAudioDefinition.h"

void FDLLAudioRegistry::Register(const UDLLAudioDefinition* Definition)
{
	if (!Definition || !Definition->EventTag.IsValid())
	{
		UE_LOG(LogDLLAudio, Warning, TEXT("DLLAudio: ignoring definition %s with no EventTag."), *GetNameSafe(Definition));
		return;
	}

	TArray<TWeakObjectPtr<const UDLLAudioDefinition>>& Bucket = ByTag.FindOrAdd(Definition->EventTag);
	Bucket.AddUnique(Definition);
}

void FDLLAudioRegistry::Unregister(const UDLLAudioDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	if (TArray<TWeakObjectPtr<const UDLLAudioDefinition>>* Bucket = ByTag.Find(Definition->EventTag))
	{
		Bucket->Remove(Definition);
		if (Bucket->IsEmpty())
		{
			ByTag.Remove(Definition->EventTag);
		}
	}
}

void FDLLAudioRegistry::Reset()
{
	ByTag.Reset();
}

int32 FDLLAudioRegistry::Num() const
{
	int32 Count = 0;
	for (const TPair<FGameplayTag, TArray<TWeakObjectPtr<const UDLLAudioDefinition>>>& Pair : ByTag)
	{
		Count += Pair.Value.Num();
	}
	return Count;
}

const UDLLAudioDefinition* FDLLAudioRegistry::FindExact(const FGameplayTag& EventTag, const FGameplayTagContainer& Context) const
{
	const TArray<TWeakObjectPtr<const UDLLAudioDefinition>>* Bucket = ByTag.Find(EventTag);
	if (!Bucket)
	{
		return nullptr;
	}

	const UDLLAudioDefinition* Best = nullptr;
	int32 BestSpecificity = -1;
	for (const TWeakObjectPtr<const UDLLAudioDefinition>& Weak : *Bucket)
	{
		const UDLLAudioDefinition* Candidate = Weak.Get();
		if (!Candidate || !Context.HasAll(Candidate->RequiredContextTags))
		{
			continue;
		}

		// Most required tags wins; ties keep the first registered (stable, deterministic).
		const int32 Specificity = Candidate->RequiredContextTags.Num();
		if (Specificity > BestSpecificity)
		{
			Best = Candidate;
			BestSpecificity = Specificity;
		}
	}
	return Best;
}

const UDLLAudioDefinition* FDLLAudioRegistry::Find(const FGameplayTag& EventTag, const FGameplayTagContainer& Context) const
{
	FGameplayTag Current = EventTag;
	while (Current.IsValid())
	{
		if (const UDLLAudioDefinition* Found = FindExact(Current, Context))
		{
			return Found;
		}
		Current = Current.RequestDirectParent();
	}
	return nullptr;
}

bool FDLLAudioDedupFilter::Admit(const FGameplayTag& EventTag, int32 DedupKey, double Now, double WindowSeconds)
{
	if (DedupKey == 0)
	{
		return true;
	}

	// Prune at most once per window so the map stays bounded without per-call full scans.
	if (LastPruneTime < 0.0 || Now - LastPruneTime >= WindowSeconds)
	{
		for (auto It = Recent.CreateIterator(); It; ++It)
		{
			if (Now - It.Value() > WindowSeconds)
			{
				It.RemoveCurrent();
			}
		}
		LastPruneTime = Now;
	}

	const uint64 Key = (uint64(GetTypeHash(EventTag)) << 32) | uint64(uint32(DedupKey));
	if (const double* Seen = Recent.Find(Key))
	{
		if (Now - *Seen <= WindowSeconds)
		{
			return false;
		}
	}
	Recent.Add(Key, Now);
	return true;
}
