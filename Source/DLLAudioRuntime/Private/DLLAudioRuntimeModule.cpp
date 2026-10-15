// Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
// SPDX-License-Identifier: Apache-2.0

#include "Modules/ModuleManager.h"
#include "DLLAudioTypes.h"
#include "DLLAudioTags.h"

DEFINE_LOG_CATEGORY(LogDLLAudio);

namespace DLLAudioTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event, "Audio.Event", "DLL Audio: root for one-shot semantic audio events.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State, "Audio.State", "DLL Audio: root for sustained audio states.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Mix, "Audio.Mix", "DLL Audio: root for global mix states.");
}

IMPLEMENT_MODULE(FDefaultModuleImpl, DLLAudioRuntime)
