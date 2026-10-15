// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DLLAudioTypes.h"
#include "DLLAudioRegistry.h"
#include "DLLAudioResolver.h"
#include "DLLAudioSubsystem.generated.h"

class UAudioComponent;
class UDLLAudioDefinition;
class UDLLAudioDefinitionLibrary;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDLLAudioEventEmittedSignature, const FDLLAudioEvent&, Event, EDLLAudioEmitResult, Result);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDLLAudioEventEmittedNative, const FDLLAudioEvent&, EDLLAudioEmitResult);

/**
 * The single entry point for gameplay audio in a world.
 *
 * Responsibilities: resolve events to definitions (registry + resolver chain), suppress duplicates, apply
 * budgets, render for the local listener(s), and broadcast every event to sinks (AI hearing, telemetry,
 * debug) — including on a dedicated server, where nothing is rendered.
 *
 * Not responsible for replication: each machine emits events from its own replicated gameplay state.
 * Event-driven; never ticks. The debug HUD uses a timer that only exists while the HUD is enabled.
 */
UCLASS()
class DLLAUDIORUNTIME_API UDLLAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Convenience accessor. Null for worlds without the subsystem (editor preview, inactive). */
	static UDLLAudioSubsystem* Get(const UObject* WorldContextObject);

	//~ UWorldSubsystem
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ End UWorldSubsystem

	// ------------------------------------------------------------------------
	// Events
	// ------------------------------------------------------------------------

	/** Emit a one-shot semantic event. Safe to call on any net mode; rendering happens only where there is a listener. */
	UFUNCTION(BlueprintCallable, Category = "DLL Audio")
	EDLLAudioEmitResult EmitAudioEvent(const FDLLAudioEvent& Event);

	/** Start a sustained state (loop). Returns an invalid handle when nothing was started. */
	UFUNCTION(BlueprintCallable, Category = "DLL Audio")
	FDLLAudioStateHandle StartAudioState(const FDLLAudioEvent& Event);

	/** Stop a sustained state, optionally fading out. */
	UFUNCTION(BlueprintCallable, Category = "DLL Audio")
	void StopAudioState(FDLLAudioStateHandle Handle, float FadeOutSeconds = 0.2f);

	/** Push parameters (MetaSound inputs) to a running state, e.g. engine RPM. */
	UFUNCTION(BlueprintCallable, Category = "DLL Audio")
	void SetAudioStateParameters(FDLLAudioStateHandle Handle, const TArray<FAudioParameter>& Parameters);

	UFUNCTION(BlueprintPure, Category = "DLL Audio")
	bool IsAudioStateActive(FDLLAudioStateHandle Handle) const;

	/** Resolve without playing. Useful for tools, tests and adapters. */
	UFUNCTION(BlueprintCallable, Category = "DLL Audio")
	FDLLAudioResolution ResolveAudioEvent(const FDLLAudioEvent& Event) const;

	// ------------------------------------------------------------------------
	// Registration (extension points)
	// ------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "DLL Audio|Registry")
	void RegisterDefinition(UDLLAudioDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category = "DLL Audio|Registry")
	void UnregisterDefinition(UDLLAudioDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category = "DLL Audio|Registry")
	void RegisterLibrary(UDLLAudioDefinitionLibrary* Library);

	UFUNCTION(BlueprintCallable, Category = "DLL Audio|Registry")
	void RegisterResolver(UDLLAudioResolver* Resolver);

	UFUNCTION(BlueprintCallable, Category = "DLL Audio|Registry")
	void UnregisterResolver(UDLLAudioResolver* Resolver);

	/** Every emitted event and its outcome. Sinks (AI hearing, telemetry) bind here. */
	UPROPERTY(BlueprintAssignable, Category = "DLL Audio")
	FDLLAudioEventEmittedSignature OnAudioEventEmitted;

	/** Native twin of OnAudioEventEmitted (no UObject binding required). */
	FDLLAudioEventEmittedNative OnAudioEventEmittedNative;

	/** Counters since world start, for the debug HUD and tests. */
	struct FStats
	{
		int32 Emitted = 0;
		int32 Rendered = 0;
		int32 NoDefinition = 0;
		int32 Deduplicated = 0;
		int32 CulledDistance = 0;
		int32 CulledBudget = 0;
		int32 PendingLoad = 0;
		int32 NoListener = 0;
		int32 ActiveStates = 0;
	};
	const FStats& GetStats() const { return Stats; }

private:
	bool HasLocalListener() const;
	bool GetLocalListenerLocations(TArray<FVector>& OutLocations) const;
	bool IsWithinAudibleDistance(const UDLLAudioDefinition& Definition, const FDLLAudioEvent& Event) const;
	bool AdmitOneShotBudget(const UDLLAudioDefinition& Definition);
	bool AreLayersLoaded(const FDLLAudioResolution& Resolution) const;
	void RequestLayerLoad(const TArray<FDLLAudioLayer>& Layers);
	TArray<UAudioComponent*> RenderLayers(const FDLLAudioEvent& Event, const FDLLAudioResolution& Resolution, bool bSustained);
	UAudioComponent* RenderLayer(const FDLLAudioEvent& Event, const UDLLAudioDefinition& Definition, const FDLLAudioLayer& Layer, bool bSustained);
	EDLLAudioEmitResult Finish(const FDLLAudioEvent& Event, EDLLAudioEmitResult Result);
	void SortResolvers();

	// Debug
	bool IsVisualDebugEnabled() const;
	void DrawEventDebug(const FDLLAudioEvent& Event, EDLLAudioEmitResult Result) const;
	void UpdateDebugHUDTimer();
	void DrawDebugHUD();

	FDLLAudioRegistry Registry;
	FDLLAudioDedupFilter DedupFilter;
	FStats Stats;

	/** Keeps registered definitions / libraries alive. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> RegisteredAssets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDLLAudioResolver>> Resolvers;

	/** Running sustained states. Components are owned by the world; we only track them. */
	struct FActiveState
	{
		TArray<TWeakObjectPtr<UAudioComponent>> Components;
	};
	TMap<int32, FActiveState> ActiveStates;
	int32 NextStateId = 1;

	/** Per-frame budget bookkeeping. */
	uint64 BudgetFrame = 0;
	int32 OneShotsThisFrame = 0;

	FTimerHandle DebugHUDTimer;
};
