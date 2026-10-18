// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UAudioComponent;
struct FDLLAudioBurstMerge;

/**
 * Bookkeeping for burst merging (FDLLAudioBurstMerge). Plain C++, no world dependency, so it is unit tested in
 * isolation. Owned by UDLLAudioSubsystem.
 *
 * A burst = (definition, variant, grid cell) with a fixed time window starting at its first voice. Variant separates
 * occurrences that share a definition but not a sound (caller layers: one fallback definition, many sounds) - they
 * must never merge into each other's voices. nullptr = the definition alone decides. The tracker only
 * decides; the subsystem renders voices and applies volume/parameters to the voices a merge returns.
 * Bounded: expired bursts are pruned on every query.
 */
class DLLAUDIORUNTIME_API FDLLAudioBurstTracker
{
public:
	/** One live voice of a burst: a rendered component and the volume it was rendered at. */
	struct FVoice
	{
		TWeakObjectPtr<UAudioComponent> Component;
		float BaseVolume = 1.0f;
	};

	/** What a merge produced: the live voices to update and the burst's counters after this occurrence. */
	struct FMergeResult
	{
		TArray<FVoice> LiveVoices;
		/** Occurrences absorbed into the burst so far, this one included. */
		int32 MergedCount = 0;
		/** Occurrences in the burst so far: voices + merged. Sent as "BurstCount". */
		int32 BurstCount = 0;
		/** Where the burst's first voice played, for debug drawing. */
		FVector Origin = FVector::ZeroVector;
	};

	/**
	 * Should this occurrence merge? True only when the burst for (Definition, Location's cell) is inside its window,
	 * already has MaxVoicesPerBurst voices, and at least one of them is still playing. On true the occurrence is
	 * counted and OutResult says which voices to update; on false the caller renders a voice and calls AddVoices.
	 */
	bool TryMerge(const void* Definition, const FVector& Location, const FDLLAudioBurstMerge& Settings, double Now, FMergeResult& OutResult,
		const void* Variant = nullptr);

	/**
	 * Record a rendered occurrence's components as one voice of its burst. Starts a new burst (window from Now)
	 * when none is open for the cell or the open one has expired or has no live voice left.
	 */
	void AddVoices(const void* Definition, const FVector& Location, const FDLLAudioBurstMerge& Settings, double Now,
		const TArray<UAudioComponent*>& Components, const TArray<float>& BaseVolumes, const void* Variant = nullptr);

	/** Volume multiple for a burst after MergedCount absorbed occurrences (1 + growth * count, capped). */
	static float VolumeScaleFor(const FDLLAudioBurstMerge& Settings, int32 MergedCount);

	int32 NumBursts() const { return Bursts.Num(); }
	void Reset() { Bursts.Reset(); }

private:
	struct FKey
	{
		const void* Definition = nullptr;
		const void* Variant = nullptr;
		FIntVector Cell = FIntVector::ZeroValue;

		bool operator==(const FKey& Other) const { return Definition == Other.Definition && Variant == Other.Variant && Cell == Other.Cell; }
		friend uint32 GetTypeHash(const FKey& Key)
		{
			return HashCombine(HashCombine(::PointerHash(Key.Definition), ::PointerHash(Key.Variant)), GetTypeHash(Key.Cell));
		}
	};

	struct FBurst
	{
		double ExpireTime = 0.0;
		FVector Origin = FVector::ZeroVector;
		/** Occurrences that rendered as voices (one per AddVoices call). */
		int32 VoiceOccurrences = 0;
		int32 MergedCount = 0;
		TArray<FVoice> Voices;
	};

	static FKey MakeKey(const void* Definition, const void* Variant, const FVector& Location, float CellSize);
	static bool HasLiveVoice(FBurst& Burst);
	void Prune(double Now);

	TMap<FKey, FBurst> Bursts;
	double LastPruneTime = -1.0;
};
