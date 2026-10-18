# DLL Audio — Architecture

## Separation of concerns

```
GAMEPLAY (game code: weapons, abilities, vehicles, AI, crowds, voxels)
        │  FDLLAudioEvent  (what happened — tag + context)
        ▼
UDLLAudioSubsystem (per world)
  1. Dedup           (EventTag, DedupKey) inside a window — predicted + confirmed plays heard once
  2. Listener check  dedicated server / audio disabled → no render (sinks still notified)
  3. Resolve         tag registry (most specific context match, parent-tag fallback) → resolver chain
  4. Pre-cull        definition MaxAudibleDistance vs every local listener (split-screen aware)
  5. Budget          per-frame one-shot cap; Priority >= AlwaysAdmitPriority bypasses it
  6. Load            soft sounds; unloaded one-shots request async load and are skipped (late > missing)
  7. Render          UAudioComponent per layer, parameters set BEFORE Play
  8. Sinks           OnAudioEventEmitted(Event, Result) on every net mode
        ▼
UNREAL AUDIO RENDERER (MetaSounds, attenuation, concurrency, submixes, modulation)
```

The engine keeps doing what it does well — voice virtualization, concurrency, spatialization. The subsystem
makes the *gameplay-informed* decisions the engine can't: which variant, whether it's worth spawning at all,
and which events matter more when the frame is crowded.

## Extension points

| Point | How | Typical use |
|---|---|---|
| **Definitions** | Data assets in libraries; `RegisterDefinition/Library` at runtime | Content, DLC, game features |
| **Resolvers** | Subclass `UDLLAudioResolver` (C++ or Blueprint), list in settings or `RegisterResolver` | Game precedence rules ("authored sound always plays, surface layer under it"), difficulty/accessibility swaps |
| **Sinks** | Bind `OnAudioEventEmitted` / `OnAudioEventEmittedNative` | AI hearing stimulus (server), subtitles / visualised sound for accessibility, telemetry |
| **Sustained states** | `StartAudioState` → handle → `SetAudioStateParameters` | Engines (RPM), shields, beams, ambient machinery |

## Networking model

The plugin **does not replicate**. In a networked game the server decides *what happened* and replicates
gameplay state the way it already does (Gameplay Cues, OnRep, multicast RPCs, movement). Every machine then
emits the audio event locally, so each client hears it from its own position with its own occlusion and
mix. Use `DedupKey` (prediction key, shot id) so a locally predicted sound isn't replayed when the
confirmation arrives.

AI hearing must not depend on rendered audio: bind a sink on the server and convert events to perception
stimuli there. Sinks fire even where nothing is rendered.

## Performance rules

- No Tick. The debug HUD uses a timer that exists only while the HUD is enabled.
- Nothing is spawned for culled events; pre-cull happens before load and component creation.
- Definitions are small; sounds are soft references, preloaded per definition (`bPreloadSounds`) only on
  machines that can hear.
- The registry lookup is a map hit per tag level; resolution cost is O(definitions for that tag).
- Burst merging (opt-in per definition) runs after the distance cull and before the budget, so an absorbed
  occurrence costs no component and no budget slot. Order: dedup -> listener -> resolve -> distance -> **burst** ->
  budget -> load -> render. Bursts are keyed by (definition, grid cell), use a fixed window, and are pruned on
  query. A merge only joins a voice that is still playing; sinks still get every occurrence (`Merged`).

## Planned modules (see ROADMAP.md)

| Module | Depends on | Purpose |
|---|---|---|
| `DLLAudioRuntime` | Engine, GameplayTags, AudioExtensions | This core |
| `DLLAudioAcoustics` | Runtime | Acoustic zones (wrapping Audio Gameplay Volumes), portal graph, batched async direct-path occlusion, acoustic materials keyed by gameplay tag, provider interface (native solver first; Steam Audio as an optional provider) |
| `DLLAudioMix` | Runtime, AudioModulation | `Audio.Mix.*` states driving modulation buses |
| `DLLAudioGAS` | Runtime, GameplayAbilities | Gameplay Cue → audio event bridge, prediction-key dedup |
| `DLLAudioMass` | Runtime, MassEntity | Aggregate crowd emitters per spatial cell instead of per-entity sounds |
| `DLLAudioEditor` | Runtime | Definition validation, templates, debugger panel |

Integrations with *specific* crowd or voxel plugins live in the adopting game (or a tiny bridge module),
never in this plugin, so dll-* plugins never depend on each other.
