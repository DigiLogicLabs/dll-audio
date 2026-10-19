# DLL Audio

**Gameplay-tag driven audio event routing for Unreal Engine 5.** Open source (Apache-2.0) by
[Digi Logic Labs](https://digilogiclabs.com) ([GitHub](https://github.com/DigiLogicLabs)). Built for, and developed alongside, our own UE games.

> **Status: 0.1.0 (beta) — foundation only.** The core event router is here. Acoustic propagation,
> GAS and Mass integration are on the [roadmap](docs/ROADMAP.md). APIs may change before 1.0.

## Why

Most UE projects end up with dozens of `PlaySoundAtLocation` calls scattered through weapons, abilities,
vehicles and AI, each with its own (or no) attenuation, concurrency and variation logic. DLL Audio replaces
that with one idea:

> Gameplay code says **what happened** (a semantic `Audio.Event.*` tag plus context).
> Data decides **how it sounds**. One subsystem decides **whether and where it plays** for each local listener.

## What's in 0.1.0

| Piece | What it does |
|---|---|
| `FDLLAudioEvent` | The semantic event: tag, context tags, location/attachment, intensity, speed, seed, dedup key, MetaSound parameters. |
| `UDLLAudioDefinition` | Data asset: layers (soft sounds, volume, pitch range), attenuation/concurrency overrides, attachment, priority, pre-cull distance, acoustic tier. |
| `UDLLAudioDefinitionLibrary` | A list of definitions registered together (project settings or at runtime). |
| `UDLLAudioSubsystem` | World subsystem. Resolve → dedup → listener check → distance pre-cull → per-frame budget → render. Sustained states with handles + live parameters. Never ticks. |
| `UDLLAudioResolver` | Extension point: a chain of resolvers that can replace the definition or add layers — where game-specific rules live. |
| Sinks | `OnAudioEventEmitted` fires for **every** event with its outcome, on every net mode — wire AI hearing, telemetry, subtitles. |
| Debug | `dll.Audio.Debug 1` world markers (green rendered / orange culled / red no definition), `2` adds a screen HUD. |

## Install

1. Clone into your project's `Plugins/` folder:
   `git clone https://github.com/DigiLogicLabs/dll-audio.git Plugins/dll-audio`
2. Enable **DLL Audio** in the Plugins window (or add `"DLLAudio"` to your `.uproject`).
3. C++ users: add `"DLLAudioRuntime"` to your module's `Build.cs` dependencies.

Tested with **Unreal Engine 5.7**.

## Quick start

1. Add your event tags under `Audio.Event` (e.g. `Audio.Event.Weapon.Fire`) in your project's tag config.
2. Create a **DLL Audio Definition** data asset: set `EventTag`, add a layer with your sound.
3. Create a **DLL Audio Definition Library**, add the definition, and add the library under
   *Project Settings → Plugins → DLL Audio → Definition Libraries*. For packaged builds, add the
   `DLLAudioDefinitionLibrary` primary asset type to the Asset Manager (or the library to Always Cook).
4. Emit:

```cpp
#include "DLLAudioSubsystem.h"

FDLLAudioEvent Event;
Event.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Audio.Event.Weapon.Fire"));
Event.Location = MuzzleLocation;
Event.SourceActor = this;
Event.DedupKey = ShotId; // optional: predicted + confirmed plays of the same shot are heard once

if (UDLLAudioSubsystem* Audio = UDLLAudioSubsystem::Get(this))
{
    Audio->EmitAudioEvent(Event);
}
```

Context picks variants: add `Environment.Material.Metal` to `Event.ContextTags`, and a definition whose
`RequiredContextTags` contains it will win over the generic one.

## Design rules

- **The plugin never replicates.** Each machine emits events from its own replicated gameplay state
  (Gameplay Cues, OnRep, multicast). Rendering is listener-local; a dedicated server renders nothing but
  still broadcasts to sinks.
- **The core depends only on the engine and GameplayTags.** Integrations (GAS, Mass, acoustics) are separate
  optional modules.
- **Your game vocabulary stays in your game.** The plugin owns only the `Audio.Event` / `Audio.State` /
  `Audio.Mix` roots.

## Documentation

[docs/README.md](docs/README.md) is the index: [PRD](docs/PRD.md) (goals and requirements),
[architecture](docs/ARCHITECTURE.md), [roadmap](docs/ROADMAP.md), [decisions](docs/DECISIONS.md),
[integration guide](docs/INTEGRATION_GUIDE.md), [testing](docs/TESTING.md), [research](docs/RESEARCH.md).

**Working with AI assistants:** [`AGENTS.md`](AGENTS.md) is the canonical rule set (Claude Code, Codex,
Copilot all read it); the session workflow is in [docs/AI_WORKFLOW.md](docs/AI_WORKFLOW.md).

## Tests

Automation tests live under `DLLAudio.*` (Session Frontend → Automation, or
`-ExecCmds="Automation RunTests DLLAudio"`).

## Sister plugins

Part of the Digi Logic Labs `dll-*` plugin family — see [docs/DLL_PLUGIN_STANDARD.md](docs/DLL_PLUGIN_STANDARD.md).

## License

Free and open source under the [Apache License 2.0](LICENSE) (see also [NOTICE](NOTICE)). In plain words: use it
in any project, personal or commercial, change it, ship it in your games and redistribute it, with no fees or
royalties. When you redistribute the plugin or a modified copy, keep the `LICENSE` and `NOTICE` text with it (for a
shipped game, its credits or documentation is enough) and mark the files you changed. The license includes a patent
grant for this code; it does not grant use of the Digi Logic Labs name or logos, and the software comes without
warranty. Contributions welcome - see [CONTRIBUTING.md](CONTRIBUTING.md).

More from Digi Logic Labs: [digilogiclabs.com](https://digilogiclabs.com).
