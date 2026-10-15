// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DLLAudioTypes.h"
#include "DLLAudioResolver.generated.h"

class UDLLAudioDefinition;

/** The result of resolving one event: the chosen definition plus any layers resolvers added. */
USTRUCT(BlueprintType)
struct DLLAUDIORUNTIME_API FDLLAudioResolution
{
	GENERATED_BODY()

	/** Chosen definition. Pre-filled by the registry; a resolver may replace it. */
	UPROPERTY(BlueprintReadWrite, Category = "DLL Audio")
	TObjectPtr<const UDLLAudioDefinition> Definition = nullptr;

	/**
	 * Extra layers rendered with the definition's settings (attenuation, concurrency, attachment).
	 * Example: a game adds a surface layer ("what got hit") under an authored impact ("what hit it").
	 */
	UPROPERTY(BlueprintReadWrite, Category = "DLL Audio")
	TArray<FDLLAudioLayer> ExtraLayers;
};

/**
 * Extension point: a step in the resolution chain, run after the built-in tag registry.
 *
 * Resolvers are instantiated from UDLLAudioSettings::ResolverClasses (or registered at runtime) and run in
 * ascending Order. Each may inspect the event, replace the definition, or append layers. Return true to
 * stop the chain. Game-specific precedence rules belong here, not in the core.
 */
UCLASS(Abstract, Blueprintable)
class DLLAUDIORUNTIME_API UDLLAudioResolver : public UObject
{
	GENERATED_BODY()

public:
	/** Lower runs first. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DLL Audio")
	int32 Order = 0;

	/** Return true to stop later resolvers from running. */
	UFUNCTION(BlueprintNativeEvent, Category = "DLL Audio")
	bool Resolve(const FDLLAudioEvent& Event, UPARAM(ref) FDLLAudioResolution& InOutResolution) const;

	virtual bool Resolve_Implementation(const FDLLAudioEvent& Event, FDLLAudioResolution& InOutResolution) const { return false; }

	virtual UWorld* GetWorld() const override;
};
