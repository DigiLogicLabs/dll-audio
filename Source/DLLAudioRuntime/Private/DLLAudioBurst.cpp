// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "DLLAudioBurst.h"
#include "DLLAudioTypes.h"
#include "Components/AudioComponent.h"

FDLLAudioBurstTracker::FKey FDLLAudioBurstTracker::MakeKey(const void* Definition, const void* Variant, const FVector& Location, float CellSize)
{
	const double Cell = FMath::Max(double(CellSize), 1.0);
	FKey Key;
	Key.Definition = Definition;
	Key.Variant = Variant;
	Key.Cell = FIntVector(
		FMath::FloorToInt32(Location.X / Cell),
		FMath::FloorToInt32(Location.Y / Cell),
		FMath::FloorToInt32(Location.Z / Cell));
	return Key;
}

bool FDLLAudioBurstTracker::HasLiveVoice(FBurst& Burst)
{
	// A finished one-shot auto-destroys its component, which invalidates the weak pointer.
	Burst.Voices.RemoveAll([](const FVoice& Voice) { return !Voice.Component.IsValid(); });
	return !Burst.Voices.IsEmpty();
}

void FDLLAudioBurstTracker::Prune(double Now)
{
	if (Now == LastPruneTime)
	{
		return;
	}
	LastPruneTime = Now;

	for (auto It = Bursts.CreateIterator(); It; ++It)
	{
		if (It.Value().ExpireTime <= Now)
		{
			It.RemoveCurrent();
		}
	}
}

float FDLLAudioBurstTracker::VolumeScaleFor(const FDLLAudioBurstMerge& Settings, int32 MergedCount)
{
	const float Scale = 1.0f + FMath::Max(Settings.VolumeGrowthPerMergedEvent, 0.0f) * float(FMath::Max(MergedCount, 0));
	return FMath::Min(Scale, FMath::Max(Settings.MaxVolumeScale, 1.0f));
}

bool FDLLAudioBurstTracker::TryMerge(const void* Definition, const FVector& Location, const FDLLAudioBurstMerge& Settings, double Now, FMergeResult& OutResult,
	const void* Variant)
{
	Prune(Now);

	FBurst* Burst = Bursts.Find(MakeKey(Definition, Variant, Location, Settings.CellSize));
	if (!Burst || Burst->ExpireTime <= Now)
	{
		return false;
	}

	// Room for another voice of its own, or nothing left playing to merge into: the caller renders.
	if (Burst->VoiceOccurrences < FMath::Max(Settings.MaxVoicesPerBurst, 1) || !HasLiveVoice(*Burst))
	{
		return false;
	}

	++Burst->MergedCount;
	OutResult.LiveVoices = Burst->Voices;
	OutResult.MergedCount = Burst->MergedCount;
	OutResult.BurstCount = Burst->VoiceOccurrences + Burst->MergedCount;
	OutResult.Origin = Burst->Origin;
	return true;
}

void FDLLAudioBurstTracker::AddVoices(const void* Definition, const FVector& Location, const FDLLAudioBurstMerge& Settings, double Now,
	const TArray<UAudioComponent*>& Components, const TArray<float>& BaseVolumes, const void* Variant)
{
	if (Components.IsEmpty())
	{
		return;
	}

	Prune(Now);

	FBurst& Burst = Bursts.FindOrAdd(MakeKey(Definition, Variant, Location, Settings.CellSize));
	if (Burst.ExpireTime <= Now || !HasLiveVoice(Burst))
	{
		Burst = FBurst();
		Burst.ExpireTime = Now + FMath::Max(double(Settings.WindowSeconds), 0.0);
		Burst.Origin = Location;
	}

	++Burst.VoiceOccurrences;
	for (int32 Index = 0; Index < Components.Num(); ++Index)
	{
		FVoice& Voice = Burst.Voices.AddDefaulted_GetRef();
		Voice.Component = Components[Index];
		Voice.BaseVolume = BaseVolumes.IsValidIndex(Index) ? BaseVolumes[Index] : 1.0f;
	}
}
