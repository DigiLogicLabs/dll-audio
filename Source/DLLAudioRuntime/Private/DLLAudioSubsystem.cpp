// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "DLLAudioSubsystem.h"
#include "DLLAudioDefinition.h"
#include "DLLAudioSettings.h"
#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace DLLAudioPrivate
{
	/** -1 = use settings, 0 = off, 1 = events, 2 = events + HUD. */
	static int32 GDebugMode = -1;
	static FAutoConsoleVariableRef CVarDebug(
		TEXT("dll.Audio.Debug"),
		GDebugMode,
		TEXT("DLL Audio visual debug. -1 = use Project Settings, 0 = off, 1 = event markers, 2 = markers + screen HUD."),
		ECVF_Cheat);

	static const FName ParamIntensity(TEXT("Intensity"));
	static const FName ParamSpeed(TEXT("Speed"));
	static const FName ParamSeed(TEXT("Seed"));
	static const FName ParamBurstCount(TEXT("BurstCount"));

	/** -1 = each definition's BurstMerge decides, 0 = burst merging off everywhere (A/B against plain playback). */
	static int32 GBurstMergeMode = -1;
	static FAutoConsoleVariableRef CVarBurstMerge(
		TEXT("dll.Audio.BurstMerge"),
		GBurstMergeMode,
		TEXT("DLL Audio burst merging. -1 = per definition (UDLLAudioDefinition::BurstMerge), 0 = off everywhere."),
		ECVF_Cheat);

	/** On-screen key range reserved for this system. */
	static constexpr int32 HUDKeyBase = 0x0D11A000;
}

UDLLAudioSubsystem* UDLLAudioSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UDLLAudioSubsystem>() : nullptr;
}

bool UDLLAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UDLLAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UDLLAudioSettings* Settings = GetDefault<UDLLAudioSettings>();

	for (const TSoftObjectPtr<UDLLAudioDefinitionLibrary>& LibraryRef : Settings->DefinitionLibraries)
	{
		// Definitions are small (sounds stay soft), so a synchronous load at world start is acceptable.
		if (UDLLAudioDefinitionLibrary* Library = LibraryRef.LoadSynchronous())
		{
			RegisterLibrary(Library);
		}
		else if (!LibraryRef.IsNull())
		{
			UE_LOG(LogDLLAudio, Warning, TEXT("DLLAudio: definition library %s failed to load (not cooked?)."), *LibraryRef.ToString());
		}
	}

	for (const TSoftClassPtr<UDLLAudioResolver>& ResolverClassRef : Settings->ResolverClasses)
	{
		if (UClass* ResolverClass = ResolverClassRef.LoadSynchronous())
		{
			if (!ResolverClass->HasAnyClassFlags(CLASS_Abstract))
			{
				RegisterResolver(NewObject<UDLLAudioResolver>(this, ResolverClass));
			}
		}
	}

	UE_LOG(LogDLLAudio, Log, TEXT("DLLAudio: %d definitions, %d resolvers registered for %s."),
		Registry.Num(), Resolvers.Num(), *GetNameSafe(GetWorld()));
}

void UDLLAudioSubsystem::Deinitialize()
{
	for (TPair<int32, FActiveState>& Pair : ActiveStates)
	{
		for (const TWeakObjectPtr<UAudioComponent>& Weak : Pair.Value.Components)
		{
			if (UAudioComponent* Component = Weak.Get())
			{
				Component->Stop();
			}
		}
	}
	ActiveStates.Reset();
	BurstTracker.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DebugHUDTimer);
	}

	Registry.Reset();
	RegisteredAssets.Reset();
	Resolvers.Reset();
	Super::Deinitialize();
}

void UDLLAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UpdateDebugHUDTimer();
}

// ============================================================================
// Registration
// ============================================================================

void UDLLAudioSubsystem::RegisterDefinition(UDLLAudioDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}

	Registry.Register(Definition);
	RegisteredAssets.AddUnique(Definition);

	// Preload is skipped where nothing can be heard (dedicated server).
	if (Definition->bPreloadSounds && HasLocalListener())
	{
		RequestLayerLoad(Definition->Layers);
	}
}

void UDLLAudioSubsystem::UnregisterDefinition(UDLLAudioDefinition* Definition)
{
	Registry.Unregister(Definition);
	RegisteredAssets.Remove(Definition);
}

void UDLLAudioSubsystem::RegisterLibrary(UDLLAudioDefinitionLibrary* Library)
{
	if (!Library)
	{
		return;
	}

	RegisteredAssets.AddUnique(Library);
	for (UDLLAudioDefinition* Definition : Library->Definitions)
	{
		RegisterDefinition(Definition);
	}
}

void UDLLAudioSubsystem::RegisterResolver(UDLLAudioResolver* Resolver)
{
	if (Resolver)
	{
		Resolvers.AddUnique(Resolver);
		SortResolvers();
	}
}

void UDLLAudioSubsystem::UnregisterResolver(UDLLAudioResolver* Resolver)
{
	Resolvers.Remove(Resolver);
}

void UDLLAudioSubsystem::SortResolvers()
{
	Resolvers.RemoveAll([](const TObjectPtr<UDLLAudioResolver>& R) { return R == nullptr; });
	Resolvers.StableSort([](const UDLLAudioResolver& A, const UDLLAudioResolver& B) { return A.Order < B.Order; });
}

// ============================================================================
// Resolution
// ============================================================================

FDLLAudioResolution UDLLAudioSubsystem::ResolveAudioEvent(const FDLLAudioEvent& Event) const
{
	FDLLAudioResolution Resolution;
	Resolution.Definition = Registry.Find(Event.EventTag, Event.ContextTags);

	// Caller-chosen layers ride along as extra layers: resolvers may keep, rescale or drop them, and they only
	// render if some definition is resolved (see FDLLAudioEvent::CallerLayers).
	Resolution.ExtraLayers = Event.CallerLayers;

	for (const TObjectPtr<UDLLAudioResolver>& Resolver : Resolvers)
	{
		if (Resolver && Resolver->Resolve(Event, Resolution))
		{
			break;
		}
	}
	return Resolution;
}

// ============================================================================
// Emission
// ============================================================================

EDLLAudioEmitResult UDLLAudioSubsystem::EmitAudioEvent(const FDLLAudioEvent& Event)
{
	++Stats.Emitted;
	UpdateDebugHUDTimer();

	const UDLLAudioSettings* Settings = GetDefault<UDLLAudioSettings>();
	const UWorld* World = GetWorld();
	if (!World)
	{
		return EDLLAudioEmitResult::NoListener;
	}

	if (!DedupFilter.Admit(Event.EventTag, Event.DedupKey, World->GetTimeSeconds(), Settings->DedupWindowSeconds))
	{
		return Finish(Event, EDLLAudioEmitResult::Deduplicated);
	}

	// Sinks (AI hearing etc.) still get the event on a dedicated server; only rendering is skipped.
	if (!HasLocalListener())
	{
		return Finish(Event, EDLLAudioEmitResult::NoListener);
	}

	const FDLLAudioResolution Resolution = ResolveAudioEvent(Event);
	const UDLLAudioDefinition* Definition = Resolution.Definition;
	if (!Definition)
	{
		return Finish(Event, EDLLAudioEmitResult::NoDefinition);
	}

	if (!IsWithinAudibleDistance(*Definition, Event))
	{
		return Finish(Event, EDLLAudioEmitResult::CulledDistance);
	}

	// Burst merging sits before the budget: an absorbed occurrence costs no voice and no budget slot.
	const bool bBurstMerge = IsBurstMergeActive(*Definition);
	const FVector SourceLocation = Event.AttachComponent ? Event.AttachComponent->GetSocketLocation(Event.AttachSocket) : Event.Location;
	// Caller layers share one definition across many sounds: only the same sound merges into the same voices.
	const void* BurstVariant = Event.CallerLayers.IsEmpty() ? nullptr : Event.CallerLayers[0].Sound.Get();
	if (bBurstMerge)
	{
		FDLLAudioBurstTracker::FMergeResult Merge;
		if (BurstTracker.TryMerge(Definition, SourceLocation, Definition->BurstMerge, World->GetTimeSeconds(), Merge, BurstVariant))
		{
			ApplyBurstMerge(*Definition, Merge);
#if ENABLE_DRAW_DEBUG
			if (IsVisualDebugEnabled())
			{
				DrawDebugLine(World, SourceLocation, Merge.Origin, FColor::Cyan, false, GetDefault<UDLLAudioSettings>()->DebugEventDrawDuration);
			}
#endif
			return Finish(Event, EDLLAudioEmitResult::Merged);
		}
	}

	if (!AdmitOneShotBudget(*Definition))
	{
		return Finish(Event, EDLLAudioEmitResult::CulledBudget);
	}

	if (!AreLayersLoaded(Resolution))
	{
		// A late one-shot is worse than a missing one: request the load and skip this occurrence.
		RequestLayerLoad(Definition->Layers);
		RequestLayerLoad(Resolution.ExtraLayers);
		return Finish(Event, EDLLAudioEmitResult::PendingLoad);
	}

	const TArray<UAudioComponent*> Components = RenderLayers(Event, Resolution, /*bSustained*/ false);
	if (bBurstMerge)
	{
		RecordBurstVoices(*Definition, SourceLocation, Components, BurstVariant);
	}
	return Finish(Event, EDLLAudioEmitResult::Rendered);
}

FDLLAudioStateHandle UDLLAudioSubsystem::StartAudioState(const FDLLAudioEvent& Event)
{
	FDLLAudioStateHandle Handle;
	++Stats.Emitted;

	if (!HasLocalListener())
	{
		Finish(Event, EDLLAudioEmitResult::NoListener);
		return Handle;
	}

	const FDLLAudioResolution Resolution = ResolveAudioEvent(Event);
	if (!Resolution.Definition)
	{
		Finish(Event, EDLLAudioEmitResult::NoDefinition);
		return Handle;
	}

	// Sustained sounds are expected to be audible for a long time; load synchronously rather than drop.
	for (const FDLLAudioLayer& Layer : Resolution.Definition->Layers)
	{
		Layer.Sound.LoadSynchronous();
	}
	for (const FDLLAudioLayer& Layer : Resolution.ExtraLayers)
	{
		Layer.Sound.LoadSynchronous();
	}

	TArray<UAudioComponent*> Components = RenderLayers(Event, Resolution, /*bSustained*/ true);
	if (Components.IsEmpty())
	{
		Finish(Event, EDLLAudioEmitResult::NoDefinition);
		return Handle;
	}

	Handle.Id = NextStateId++;
	FActiveState& State = ActiveStates.Add(Handle.Id);
	for (UAudioComponent* Component : Components)
	{
		State.Components.Add(Component);
	}
	Stats.ActiveStates = ActiveStates.Num();

	Finish(Event, EDLLAudioEmitResult::Rendered);
	return Handle;
}

void UDLLAudioSubsystem::StopAudioState(FDLLAudioStateHandle Handle, float FadeOutSeconds)
{
	FActiveState State;
	if (!ActiveStates.RemoveAndCopyValue(Handle.Id, State))
	{
		return;
	}

	for (const TWeakObjectPtr<UAudioComponent>& Weak : State.Components)
	{
		if (UAudioComponent* Component = Weak.Get())
		{
			// Sustained components are not auto-destroyed while playing; let them clean up once stopped.
			Component->bAutoDestroy = true;
			if (FadeOutSeconds > 0.0f)
			{
				Component->FadeOut(FadeOutSeconds, 0.0f);
			}
			else
			{
				Component->Stop();
			}
		}
	}
	Stats.ActiveStates = ActiveStates.Num();
}

void UDLLAudioSubsystem::SetAudioStateParameters(FDLLAudioStateHandle Handle, const TArray<FAudioParameter>& Parameters)
{
	if (const FActiveState* State = ActiveStates.Find(Handle.Id))
	{
		for (const TWeakObjectPtr<UAudioComponent>& Weak : State->Components)
		{
			if (UAudioComponent* Component = Weak.Get())
			{
				TArray<FAudioParameter> Copy = Parameters;
				Component->SetParameters(MoveTemp(Copy));
			}
		}
	}
}

bool UDLLAudioSubsystem::IsAudioStateActive(FDLLAudioStateHandle Handle) const
{
	if (const FActiveState* State = ActiveStates.Find(Handle.Id))
	{
		for (const TWeakObjectPtr<UAudioComponent>& Weak : State->Components)
		{
			if (Weak.IsValid())
			{
				return true;
			}
		}
	}
	return false;
}

EDLLAudioEmitResult UDLLAudioSubsystem::Finish(const FDLLAudioEvent& Event, EDLLAudioEmitResult Result)
{
	switch (Result)
	{
	case EDLLAudioEmitResult::Rendered:       ++Stats.Rendered; break;
	case EDLLAudioEmitResult::NoDefinition:   ++Stats.NoDefinition; break;
	case EDLLAudioEmitResult::Deduplicated:   ++Stats.Deduplicated; break;
	case EDLLAudioEmitResult::CulledDistance: ++Stats.CulledDistance; break;
	case EDLLAudioEmitResult::CulledBudget:   ++Stats.CulledBudget; break;
	case EDLLAudioEmitResult::PendingLoad:    ++Stats.PendingLoad; break;
	case EDLLAudioEmitResult::NoListener:     ++Stats.NoListener; break;
	case EDLLAudioEmitResult::Merged:         ++Stats.Merged; break;
	}

	DrawEventDebug(Event, Result);
	OnAudioEventEmittedNative.Broadcast(Event, Result);
	OnAudioEventEmitted.Broadcast(Event, Result);
	return Result;
}

// ============================================================================
// Listener / budget
// ============================================================================

bool UDLLAudioSubsystem::HasLocalListener() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !World->bAllowAudioPlayback)
	{
		return false;
	}
	return World->GetAudioDeviceRaw() != nullptr;
}

bool UDLLAudioSubsystem::GetLocalListenerLocations(TArray<FVector>& OutLocations) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// One listener per local player (split-screen aware).
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController())
		{
			FVector Location, Front, Right;
			PC->GetAudioListenerPosition(Location, Front, Right);
			OutLocations.Add(Location);
		}
	}
	return !OutLocations.IsEmpty();
}

bool UDLLAudioSubsystem::IsWithinAudibleDistance(const UDLLAudioDefinition& Definition, const FDLLAudioEvent& Event) const
{
	if (Definition.MaxAudibleDistance <= 0.0f)
	{
		return true;
	}

	const FVector Source = Event.AttachComponent ? Event.AttachComponent->GetSocketLocation(Event.AttachSocket) : Event.Location;
	TArray<FVector> Found;
	if (!GetLocalListenerLocations(Found))
	{
		// No player controller yet (loading, spectator setup): don't silently cull.
		return true;
	}

	const double MaxDistSq = FMath::Square(double(Definition.MaxAudibleDistance));
	for (const FVector& Listener : Found)
	{
		if (FVector::DistSquared(Listener, Source) <= MaxDistSq)
		{
			return true;
		}
	}
	return false;
}

bool UDLLAudioSubsystem::AdmitOneShotBudget(const UDLLAudioDefinition& Definition)
{
	const UDLLAudioSettings* Settings = GetDefault<UDLLAudioSettings>();
	if (Definition.PlaybackKind != EDLLAudioPlaybackKind::OneShot || Settings->MaxOneShotsPerFrame <= 0)
	{
		return true;
	}

	if (BudgetFrame != GFrameCounter)
	{
		BudgetFrame = GFrameCounter;
		OneShotsThisFrame = 0;
	}

	if (Definition.Priority >= Settings->AlwaysAdmitPriority || OneShotsThisFrame < Settings->MaxOneShotsPerFrame)
	{
		++OneShotsThisFrame;
		return true;
	}
	return false;
}

// ============================================================================
// Burst merging
// ============================================================================

bool UDLLAudioSubsystem::IsBurstMergeActive(const UDLLAudioDefinition& Definition) const
{
	return DLLAudioPrivate::GBurstMergeMode != 0
		&& Definition.BurstMerge.bEnabled
		&& Definition.PlaybackKind == EDLLAudioPlaybackKind::OneShot;
}

void UDLLAudioSubsystem::ApplyBurstMerge(const UDLLAudioDefinition& Definition, const FDLLAudioBurstTracker::FMergeResult& Merge) const
{
	const FDLLAudioBurstMerge& Settings = Definition.BurstMerge;
	const bool bGrowVolume = Settings.VolumeGrowthPerMergedEvent > 0.0f;
	const float VolumeScale = FDLLAudioBurstTracker::VolumeScaleFor(Settings, Merge.MergedCount);

	for (const FDLLAudioBurstTracker::FVoice& Voice : Merge.LiveVoices)
	{
		UAudioComponent* Component = Voice.Component.Get();
		if (!Component)
		{
			continue;
		}
		if (bGrowVolume)
		{
			Component->SetVolumeMultiplier(Voice.BaseVolume * VolumeScale);
		}
		if (Settings.bSendBurstParameters)
		{
			Component->SetIntParameter(DLLAudioPrivate::ParamBurstCount, Merge.BurstCount);
		}
	}
}

void UDLLAudioSubsystem::RecordBurstVoices(const UDLLAudioDefinition& Definition, const FVector& Location, const TArray<UAudioComponent*>& Components,
	const void* Variant)
{
	const UWorld* World = GetWorld();
	if (!World || Components.IsEmpty())
	{
		return;
	}

	TArray<float> BaseVolumes;
	BaseVolumes.Reserve(Components.Num());
	for (const UAudioComponent* Component : Components)
	{
		BaseVolumes.Add(Component ? Component->VolumeMultiplier : 1.0f);
	}
	BurstTracker.AddVoices(&Definition, Location, Definition.BurstMerge, World->GetTimeSeconds(), Components, BaseVolumes, Variant);
}

// ============================================================================
// Loading / rendering
// ============================================================================

bool UDLLAudioSubsystem::AreLayersLoaded(const FDLLAudioResolution& Resolution) const
{
	auto AllLoaded = [](const TArray<FDLLAudioLayer>& Layers)
	{
		for (const FDLLAudioLayer& Layer : Layers)
		{
			if (!Layer.Sound.IsNull() && !Layer.Sound.IsValid())
			{
				return false;
			}
		}
		return true;
	};
	return AllLoaded(Resolution.Definition->Layers) && AllLoaded(Resolution.ExtraLayers);
}

void UDLLAudioSubsystem::RequestLayerLoad(const TArray<FDLLAudioLayer>& Layers)
{
	TArray<FSoftObjectPath> Paths;
	for (const FDLLAudioLayer& Layer : Layers)
	{
		if (!Layer.Sound.IsNull() && !Layer.Sound.IsValid())
		{
			Paths.Add(Layer.Sound.ToSoftObjectPath());
		}
	}

	if (!Paths.IsEmpty() && UAssetManager::IsInitialized())
	{
		// Handle intentionally not kept: loaded sounds stay resident through the definition's soft ref
		// until GC; the definition being registered is what matters for subsequent hits.
		UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	}
}

TArray<UAudioComponent*> UDLLAudioSubsystem::RenderLayers(const FDLLAudioEvent& Event, const FDLLAudioResolution& Resolution, bool bSustained)
{
	TArray<UAudioComponent*> Components;
	const UDLLAudioDefinition& Definition = *Resolution.Definition;

	for (const FDLLAudioLayer& Layer : Definition.Layers)
	{
		if (UAudioComponent* Component = RenderLayer(Event, Definition, Layer, bSustained))
		{
			Components.Add(Component);
		}
	}
	for (const FDLLAudioLayer& Layer : Resolution.ExtraLayers)
	{
		if (UAudioComponent* Component = RenderLayer(Event, Definition, Layer, bSustained))
		{
			Components.Add(Component);
		}
	}
	return Components;
}

UAudioComponent* UDLLAudioSubsystem::RenderLayer(const FDLLAudioEvent& Event, const UDLLAudioDefinition& Definition, const FDLLAudioLayer& Layer, bool bSustained)
{
	USoundBase* Sound = Layer.Sound.Get();
	UWorld* World = GetWorld();
	if (!Sound || !World)
	{
		return nullptr;
	}

	const bool bAttach = Definition.bAttachToSource && Event.AttachComponent != nullptr;

	FAudioDevice::FCreateComponentParams Params(World, bAttach ? Event.AttachComponent->GetOwner() : nullptr);
	Params.AttenuationSettings = Definition.Attenuation;
	if (Definition.Concurrency)
	{
		Params.ConcurrencySet.Add(Definition.Concurrency);
	}
	Params.bAutoDestroy = !bSustained;
	Params.bPlay = false;
	Params.bStopWhenOwnerDestroyed = bAttach;
	if (!bAttach)
	{
		Params.SetLocation(Event.Location);
	}

	UAudioComponent* Component = FAudioDevice::CreateComponent(Sound, Params);
	if (!Component)
	{
		return nullptr;
	}

	if (bAttach)
	{
		Component->AttachToComponent(Event.AttachComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Event.AttachSocket);
	}
	else
	{
		Component->SetWorldLocation(Event.Location);
	}

	Component->SetVolumeMultiplier(Layer.VolumeMultiplier);
	const float MinPitch = float(FMath::Min(Layer.PitchRange.X, Layer.PitchRange.Y));
	const float MaxPitch = float(FMath::Max(Layer.PitchRange.X, Layer.PitchRange.Y));
	Component->SetPitchMultiplier(FMath::IsNearlyEqual(MinPitch, MaxPitch) ? MinPitch : FMath::FRandRange(MinPitch, MaxPitch));

	// Parameters are set before Play so MetaSounds see them on their first block.
	TArray<FAudioParameter> Parameters = Event.Parameters;
	if (Definition.bSendStandardParameters)
	{
		Parameters.Emplace(DLLAudioPrivate::ParamIntensity, Event.Intensity);
		Parameters.Emplace(DLLAudioPrivate::ParamSpeed, Event.Speed);
		if (Event.VariationSeed != 0)
		{
			Parameters.Emplace(DLLAudioPrivate::ParamSeed, Event.VariationSeed);
		}
	}
	if (!bSustained && IsBurstMergeActive(Definition) && Definition.BurstMerge.bSendBurstParameters)
	{
		// A voice starts as a burst of one; merges raise it while it plays.
		Parameters.Emplace(DLLAudioPrivate::ParamBurstCount, 1);
	}
	if (!Parameters.IsEmpty())
	{
		Component->SetParameters(MoveTemp(Parameters));
	}

	Component->Play();
	return Component;
}

// ============================================================================
// Debug
// ============================================================================

bool UDLLAudioSubsystem::IsVisualDebugEnabled() const
{
	if (DLLAudioPrivate::GDebugMode >= 0)
	{
		return DLLAudioPrivate::GDebugMode > 0;
	}
	return GetDefault<UDLLAudioSettings>()->bEnableVisualDebug;
}

void UDLLAudioSubsystem::DrawEventDebug(const FDLLAudioEvent& Event, EDLLAudioEmitResult Result) const
{
#if ENABLE_DRAW_DEBUG
	const UDLLAudioSettings* Settings = GetDefault<UDLLAudioSettings>();
	if (!IsVisualDebugEnabled() || (DLLAudioPrivate::GDebugMode < 0 && !Settings->bDebugDrawEvents))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FColor Color = FColor::Green;
	switch (Result)
	{
	case EDLLAudioEmitResult::Rendered:     Color = FColor::Green; break;
	case EDLLAudioEmitResult::NoDefinition: Color = FColor::Red; break;
	case EDLLAudioEmitResult::Deduplicated: Color = FColor::Purple; break;
	case EDLLAudioEmitResult::Merged:       Color = FColor::Cyan; break;
	default:                                Color = FColor::Orange; break;
	}

	const FVector Location = Event.AttachComponent ? Event.AttachComponent->GetSocketLocation(Event.AttachSocket) : Event.Location;
	const float Duration = Settings->DebugEventDrawDuration;
	DrawDebugSphere(World, Location, 15.0f + 25.0f * FMath::Clamp(Event.Intensity, 0.0f, 2.0f), 10, Color, false, Duration);
	DrawDebugString(World, Location + FVector(0, 0, 40), FString::Printf(TEXT("%s [%s]"),
		*Event.EventTag.ToString(), *UEnum::GetDisplayValueAsText(Result).ToString()), nullptr, Color, Duration, true);
#endif
}

void UDLLAudioSubsystem::UpdateDebugHUDTimer()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const UDLLAudioSettings* Settings = GetDefault<UDLLAudioSettings>();
	const bool bWantHUD = DLLAudioPrivate::GDebugMode >= 0
		? DLLAudioPrivate::GDebugMode >= 2
		: (Settings->bEnableVisualDebug && Settings->bDebugDrawScreenHUD);

	FTimerManager& Timers = World->GetTimerManager();
	if (bWantHUD && !Timers.IsTimerActive(DebugHUDTimer))
	{
		Timers.SetTimer(DebugHUDTimer, this, &UDLLAudioSubsystem::DrawDebugHUD, 0.25f, true);
	}
	else if (!bWantHUD && Timers.IsTimerActive(DebugHUDTimer))
	{
		Timers.ClearTimer(DebugHUDTimer);
	}
}

void UDLLAudioSubsystem::DrawDebugHUD()
{
	if (!GEngine)
	{
		return;
	}

	UpdateDebugHUDTimer();

	int32 Key = DLLAudioPrivate::HUDKeyBase;
	const float Display = 0.3f; // slightly longer than the 0.25s refresh so lines don't flicker
	GEngine->AddOnScreenDebugMessage(Key++, Display, FColor::Cyan, TEXT("=== DLL AUDIO ==="));
	GEngine->AddOnScreenDebugMessage(Key++, Display, FColor::White, FString::Printf(
		TEXT("Definitions %d | Resolvers %d | Active states %d"), Registry.Num(), Resolvers.Num(), ActiveStates.Num()));
	GEngine->AddOnScreenDebugMessage(Key++, Display, FColor::Green, FString::Printf(
		TEXT("Emitted %d | Rendered %d"), Stats.Emitted, Stats.Rendered));
	GEngine->AddOnScreenDebugMessage(Key++, Display, FColor::Orange, FString::Printf(
		TEXT("Culled dist %d | budget %d | pending load %d | dedup %d"), Stats.CulledDistance, Stats.CulledBudget, Stats.PendingLoad, Stats.Deduplicated));
	GEngine->AddOnScreenDebugMessage(Key++, Display, Stats.NoDefinition > 0 ? FColor::Red : FColor::White, FString::Printf(
		TEXT("No definition %d | No listener %d"), Stats.NoDefinition, Stats.NoListener));
	GEngine->AddOnScreenDebugMessage(Key++, Display, FColor::Cyan, FString::Printf(
		TEXT("Burst merged %d | open bursts %d%s"), Stats.Merged, BurstTracker.NumBursts(),
		DLLAudioPrivate::GBurstMergeMode == 0 ? TEXT(" (OFF: dll.Audio.BurstMerge 0)") : TEXT("")));
}
