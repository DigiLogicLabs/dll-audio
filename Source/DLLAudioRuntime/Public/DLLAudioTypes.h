// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AudioParameter.h"
#include "DLLAudioTypes.generated.h"

class AActor;
class USceneComponent;
class USoundBase;

DLLAUDIORUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogDLLAudio, Log, All);

/** One rendered layer of a definition. A single logical event can render several layers. */
USTRUCT(BlueprintType)
struct DLLAUDIORUNTIME_API FDLLAudioLayer
{
	GENERATED_BODY()

	/** Sound to play: SoundWave, SoundCue or MetaSoundSource. Soft so definitions don't pull audio into memory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	/** Random pitch range; X = min, Y = max. (1,1) = no variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	FVector2D PitchRange = FVector2D(1.0, 1.0);
};

/**
 * A semantic audio event: WHAT happened, never WHICH asset plays.
 *
 * Gameplay code fills this and hands it to UDLLAudioSubsystem::EmitAudioEvent. The subsystem resolves it
 * to a UDLLAudioDefinition (tag + context), applies budgets, and renders it for the LOCAL listener(s).
 *
 * This is the local, transient form. It holds raw object pointers and is never replicated — networked
 * games carry the triggering gameplay state on their own replicated paths (cues, OnRep, multicast) and
 * emit the audio event on each machine.
 */
USTRUCT(BlueprintType)
struct DLLAUDIORUNTIME_API FDLLAudioEvent
{
	GENERATED_BODY()

	/** What happened, e.g. Audio.Event.Weapon.Fire. Resolution falls back to parent tags when no exact definition exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio", meta = (Categories = "Audio.Event"))
	FGameplayTag EventTag;

	/**
	 * Qualifiers used to pick between definitions of the same event, e.g. a surface tag, a damage type,
	 * a movement speed tier. The definition whose RequiredContextTags are all present and most numerous wins.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	FGameplayTagContainer ContextTags;

	/** World location of the event. Ignored for playback when AttachComponent is set (used for culling only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	FVector Location = FVector::ZeroVector;

	/** Actor that produced the sound (the emitter). Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	TObjectPtr<AActor> SourceActor = nullptr;

	/** Actor responsible for the event (shooter, caster). Optional; sinks such as AI hearing use it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	TObjectPtr<AActor> Instigator = nullptr;

	/** When set and the definition allows attachment, the sound follows this component/socket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	TObjectPtr<USceneComponent> AttachComponent = nullptr;

	/** Socket on AttachComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	FName AttachSocket = NAME_None;

	/** Normalised strength of the event (0..1+). Sent to the sound as the "Intensity" parameter when enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio", meta = (ClampMin = "0.0"))
	float Intensity = 1.0f;

	/** Speed of the source in cm/s, when meaningful (footsteps, collisions). Sent as "Speed". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio", meta = (ClampMin = "0.0"))
	float Speed = 0.0f;

	/** Deterministic variation seed. Sent as "Seed". 0 = let the sound pick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	int32 VariationSeed = 0;

	/**
	 * Duplicate-suppression key. When non-zero, a second event with the same EventTag + key inside the
	 * configured window is dropped. Use it for predicted sounds: the locally predicted play and the later
	 * confirmed play share a key (e.g. a prediction key or shot id) so the owner hears it once.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	int32 DedupKey = 0;

	/** Extra parameters forwarded to the sound (MetaSound inputs) before it starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	TArray<FAudioParameter> Parameters;

	/**
	 * Layers the CALLER already chose. A migration aid for legacy call sites that hold a sound asset
	 * (a weapon config's impact sound, say) and cannot yet express it as a definition.
	 *
	 * They seed FDLLAudioResolution::ExtraLayers, so resolvers see, keep, rescale or drop them, and they render
	 * with the resolved definition's settings. They never render on their own: with no definition the event is
	 * still NoDefinition. Prefer definitions for new content; this exists so adopting the plugin does not
	 * require re-authoring every existing asset first.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DLL Audio")
	TArray<FDLLAudioLayer> CallerLayers;
};

/** How a definition is played. */
UENUM(BlueprintType)
enum class EDLLAudioPlaybackKind : uint8
{
	/** Fire-and-forget. The component auto-destroys when the sound finishes. */
	OneShot,
	/** Sustained state (engine, shield hum). Started/stopped through StartAudioState/StopAudioState. */
	Sustained
};

/**
 * Acoustic quality tier requested by a definition. Stored now; consumed by the acoustics module
 * (direct-path occlusion, portal routing) when it lands. None = never processed by the solver.
 */
UENUM(BlueprintType)
enum class EDLLAudioAcousticTier : uint8
{
	None,
	Low,
	Medium,
	High
};

/** Handle to a sustained audio state started through UDLLAudioSubsystem::StartAudioState. */
USTRUCT(BlueprintType)
struct DLLAUDIORUNTIME_API FDLLAudioStateHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DLL Audio")
	int32 Id = INDEX_NONE;

	bool IsValid() const { return Id != INDEX_NONE; }
};

/** Why an event did or didn't render. Reported to sinks and counted by the debug HUD. */
UENUM(BlueprintType)
enum class EDLLAudioEmitResult : uint8
{
	Rendered,
	/** No definition matched the tag/context. */
	NoDefinition,
	/** Dropped as a duplicate of a recent event with the same DedupKey. */
	Deduplicated,
	/** Farther from every local listener than the definition's MaxAudibleDistance. */
	CulledDistance,
	/** Over the per-frame one-shot budget and below the always-admit priority. */
	CulledBudget,
	/** Sound not loaded yet; an async load was requested. */
	PendingLoad,
	/** No local listener (dedicated server, or audio disabled). Sinks still receive the event. */
	NoListener
};
