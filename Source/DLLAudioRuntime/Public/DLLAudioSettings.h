// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DLLAudioSettings.generated.h"

class UDLLAudioDefinitionLibrary;
class UDLLAudioResolver;

/** Project Settings > Plugins > DLL Audio. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "DLL Audio"))
class DLLAUDIORUNTIME_API UDLLAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDLLAudioSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	// ------------------------------------------------------------------------
	// Registry
	// ------------------------------------------------------------------------

	/**
	 * Libraries registered when a game world starts. Add them to the Asset Manager's primary asset types
	 * (type "DLLAudioDefinitionLibrary") or to Always Cook so they ship in packaged builds.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Registry")
	TArray<TSoftObjectPtr<UDLLAudioDefinitionLibrary>> DefinitionLibraries;

	/** Resolution chain steps, run after the tag registry in ascending Order. */
	UPROPERTY(Config, EditAnywhere, Category = "Registry")
	TArray<TSoftClassPtr<UDLLAudioResolver>> ResolverClasses;

	// ------------------------------------------------------------------------
	// Budget
	// ------------------------------------------------------------------------

	/** One-shot events rendered per frame before lower-priority events are dropped. 0 = unlimited. */
	UPROPERTY(Config, EditAnywhere, Category = "Budget", meta = (ClampMin = "0"))
	int32 MaxOneShotsPerFrame = 32;

	/** Definitions at or above this priority always render, ignoring the per-frame budget. */
	UPROPERTY(Config, EditAnywhere, Category = "Budget", meta = (ClampMin = "0", ClampMax = "100"))
	int32 AlwaysAdmitPriority = 80;

	/** Window in which a repeat (EventTag, DedupKey) is treated as a duplicate. */
	UPROPERTY(Config, EditAnywhere, Category = "Budget", meta = (ClampMin = "0.0", Units = "s"))
	float DedupWindowSeconds = 0.5f;

	// ------------------------------------------------------------------------
	// Debug (also: console variable dll.Audio.Debug 0/1/2)
	// ------------------------------------------------------------------------

	/** Master toggle for all DLL Audio visual debug. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	bool bEnableVisualDebug = false;

	/** Draw a marker + label at every emitted event (green rendered, orange culled, red no definition). */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableVisualDebug"))
	bool bDebugDrawEvents = true;

	/** Show emit counters on the screen HUD. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableVisualDebug"))
	bool bDebugDrawScreenHUD = true;

	/** How long event markers stay on screen. */
	UPROPERTY(Config, EditAnywhere, Category = "Debug", meta = (EditCondition = "bEnableVisualDebug", ClampMin = "0.0", Units = "s"))
	float DebugEventDrawDuration = 1.5f;
};
