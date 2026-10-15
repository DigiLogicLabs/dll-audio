// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DLLAudioTypes.h"
#include "DLLAudioDefinition.generated.h"

class USoundAttenuation;
class USoundConcurrency;

/**
 * How one semantic audio event sounds. One asset per (event tag, context) variant.
 *
 * Resolution: for an event tag T with context C, the registry picks the definition with EventTag == T whose
 * RequiredContextTags are all in C, preferring the most required tags (most specific). If none match it
 * repeats with T's parent tag, so Audio.Event.Weapon.Impact covers Audio.Event.Weapon.Impact.Ricochet
 * until a more specific definition is authored.
 */
UCLASS(BlueprintType)
class DLLAUDIORUNTIME_API UDLLAudioDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Event this definition answers. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Audio.Event"))
	FGameplayTag EventTag;

	/**
	 * All of these must be present in the event's ContextTags for this definition to match. Empty = the
	 * generic fallback for EventTag. Example: Environment.Material.Metal for a metal impact variant.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FGameplayTagContainer RequiredContextTags;

	/** Layers rendered together for one occurrence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	TArray<FDLLAudioLayer> Layers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	EDLLAudioPlaybackKind PlaybackKind = EDLLAudioPlaybackKind::OneShot;

	/** Attenuation override. Null = the sound asset's own attenuation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	TObjectPtr<USoundAttenuation> Attenuation = nullptr;

	/** Concurrency override. Null = the sound asset's own concurrency. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	TObjectPtr<USoundConcurrency> Concurrency = nullptr;

	/** Follow the event's AttachComponent/AttachSocket when provided. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	bool bAttachToSource = false;

	/** Send Intensity / Speed / Seed to the sound as parameters (MetaSound inputs of those names). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Playback")
	bool bSendStandardParameters = true;

	/**
	 * Cull before anything is loaded or spawned when every local listener is farther than this (cm).
	 * 0 = no pre-cull; the engine's attenuation still applies after spawn.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxAudibleDistance = 0.0f;

	/**
	 * Gameplay importance 0..100. Events at or above the settings' AlwaysAdmitPriority bypass the per-frame
	 * one-shot budget (critical threat cues). Below it they are dropped first when the frame is over budget.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "0", ClampMax = "100"))
	int32 Priority = 50;

	/** Load the layer sounds as soon as the definition is registered, so the first occurrence isn't dropped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budget")
	bool bPreloadSounds = true;

	/** Acoustic processing requested from the (future) acoustics module. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Acoustics")
	EDLLAudioAcousticTier AcousticTier = EDLLAudioAcousticTier::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static const FPrimaryAssetType PrimaryAssetType;
};

/**
 * A list of definitions registered together. Point UDLLAudioSettings::DefinitionLibraries at one or more
 * libraries; mods / DLC / game features can register additional libraries at runtime.
 */
UCLASS(BlueprintType)
class DLLAUDIORUNTIME_API UDLLAudioDefinitionLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DLL Audio")
	TArray<TObjectPtr<UDLLAudioDefinition>> Definitions;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static const FPrimaryAssetType PrimaryAssetType;
};
