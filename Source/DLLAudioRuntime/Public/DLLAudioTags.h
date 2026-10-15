// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NativeGameplayTags.h"

/**
 * Root tags owned by the plugin. Games add their own leaves under these roots
 * (e.g. Audio.Event.Weapon.Fire) in their own tag config — the plugin never defines game vocabulary.
 */
namespace DLLAudioTags
{
	/** Something happened (one-shot). */
	DLLAUDIORUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event);
	/** Something is ongoing (sustained loop). */
	DLLAUDIORUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State);
	/** Global mix states (combat, stealth, vehicle interior). Reserved for the mix module. */
	DLLAUDIORUNTIME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mix);
}
