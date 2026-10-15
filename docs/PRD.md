# DLL Audio — Product Requirements

> **Owner:** Digi Logic Labs · **License:** Apache-2.0 · **Status:** 0.1.0 beta (foundation)
> **Reference adopter:** MetalLegs (UE 5.7, GAS, 1-4 split-screen + online, Mass crowds, voxel terrain, vehicles)

## 1. Problem

Unreal projects accumulate audio as scattered `PlaySoundAtLocation` calls, raw `USoundBase*` fields and
per-class logic. The result, measured in our reference adopter before this plugin existed:

- 56 direct playback call sites, ~106 sound fields across ~48 headers, 3 of them passing attenuation or
  concurrency, zero submixes, zero sound classes of its own, zero acoustic zones.
- No shared vocabulary, so the same bullet hitting the same wall had **five** candidate impact sounds and
  no one place to decide.
- No budget: every event spawns a component whether or not anyone can hear it.
- No path from "what happened" to "what the AI heard" that is independent of the rendered mix.

Every studio rebuilds the same router. We want to build it once, in the open, and use it in every
Digi Logic Labs game.

## 2. Vision

> **One audio framework that serves a goblin, a modular robot, a tank, a giant boss, a destructible voxel
> tunnel and a 500-agent battlefield — without a separate sound architecture for each.**

Separate four concerns that should never be tightly coupled:

1. **Generation** — what happened (semantic events, gameplay tags, data).
2. **Propagation** — how it travels (occlusion, portals, materials, reverb).
3. **Perception** — who hears it (each local listener; AI hearing as a separate gameplay channel).
4. **Mixing** — what matters most right now (priorities, budgets, mix states).

Quality bar, as design inspiration only (not claims about their implementations): the environmental
believability of *ARC Raiders*, threat-beyond-line-of-sight of *Alien: Isolation*, combat clarity of
*Returnal* / *Helldivers 2*, material tension of *Dead Space*, cinematic transitions of *Alan Wake 2*.
See [RESEARCH.md](RESEARCH.md).

## 3. Users

| User | Needs |
|---|---|
| **Gameplay programmer** | One call to emit a sound; never touches assets, attenuation or budgets. |
| **Sound designer** | Author variants per context in data assets; MetaSound templates; no code changes. |
| **Technical sound designer** | Mix states, priorities, acoustic tiers, debug visualisation, profiling. |
| **Network programmer** | Sounds that never double-play under prediction; zero audio replication cost. |
| **Adopting studio** | Drop-in plugin, engine-only core, optional integrations, permissive license. |
| **AI contributors** (Claude, Codex, Copilot) | Checked-in context: rules, decisions, memory, prompts. See [AI_WORKFLOW.md](AI_WORKFLOW.md). |

## 4. Requirements

Priority: **P0** = required for the release that owns it, **P1** = should, **P2** = could.
Release column maps to [ROADMAP.md](ROADMAP.md).

### Events & definitions
| ID | Requirement | Pri | Release |
|---|---|---|---|
| E1 | Gameplay emits a semantic event (tag + context) without referencing any asset. | P0 | 0.1 ✅ |
| E2 | Definitions select by tag with context specificity and parent-tag fallback. | P0 | 0.1 ✅ |
| E3 | One event renders multiple layers. | P0 | 0.1 ✅ |
| E4 | Game-specific precedence rules plug in without forking (resolver chain). | P0 | 0.1 ✅ |
| E5 | Sustained states with handles and live parameters (engines, shields, beams). | P0 | 0.1 ✅ |
| E6 | Definitions validated in editor (missing sounds, unknown tags, unregistered assets). | P1 | 0.2 |
| E7 | MetaSound templates: one-shot, sustained, layered. | P1 | 0.2 |

### Networking & perception
| ID | Requirement | Pri | Release |
|---|---|---|---|
| N1 | The plugin never replicates; every machine renders from its own listener(s). | P0 | 0.1 ✅ |
| N2 | Predicted + confirmed plays of one occurrence are heard once (dedup key). | P0 | 0.1 ✅ |
| N3 | Dedicated servers render nothing and load no sounds, but still notify sinks. | P0 | 0.1 ✅ |
| N4 | Split-screen: culling considers every local listener. | P0 | 0.1 ✅ |
| N5 | AI hearing consumes gameplay events, never the rendered mix. | P0 | 0.1 ✅ (sink) |
| N6 | Gameplay Cue bridge with prediction-key dedup (`DLLAudioGAS`). | P1 | 0.3 |

### Mixing & budgets
| ID | Requirement | Pri | Release |
|---|---|---|---|
| M1 | Nothing is loaded or spawned for events no local listener can hear. | P0 | 0.1 ✅ |
| M2 | Per-frame one-shot budget; critical cues bypass it by priority. | P0 | 0.1 ✅ |
| M3 | `Audio.Mix.*` states drive Audio Modulation buses. | P1 | 0.2 |
| M4 | Perceptual priority (distance, tactical, gameplay, ownership weights). | P2 | 0.5 |

### Acoustics
| ID | Requirement | Pri | Release |
|---|---|---|---|
| A1 | Acoustic zones built on Audio Gameplay Volumes (no parallel reverb system). | P0 | 0.4 |
| A2 | Portal graph: sound reaches the listener through openings, not walls. | P0 | 0.4 |
| A3 | Batched async direct-path occlusion with a global ray budget and per-tier rates. | P0 | 0.4 |
| A4 | Acoustic materials (3-band) keyed by gameplay tag, not material names. | P1 | 0.4 |
| A5 | Dirty-region invalidation for destructible / voxel worlds. | P1 | 0.4 |
| A6 | Provider interface so Steam Audio (or others) can replace the native solver. | P2 | 0.4 |

### Scale
| ID | Requirement | Pri | Release |
|---|---|---|---|
| S1 | No Tick in the core. | P0 | 0.1 ✅ |
| S2 | Aggregate crowd emitters per spatial cell (`DLLAudioMass`). | P1 | 0.5 |
| S3 | Component pooling + emitter LOD. | P1 | 0.5 |
| S4 | Benchmark map + Insights trace markers. | P1 | 0.5 |

### Developer experience
| ID | Requirement | Pri | Release |
|---|---|---|---|
| D1 | Built-in visual debug (world markers + screen HUD, CVar + settings). | P0 | 0.1 ✅ |
| D2 | Automation tests for every pure-logic component. | P0 | ongoing |
| D3 | AI-ready repo: AGENTS.md, decisions, memory, prompts, reviewer agent. | P0 | 0.1 ✅ |

## 5. Non-goals

- **Not an audio renderer or middleware replacement.** Unreal's mixer, MetaSounds, concurrency and
  virtualization stay in charge of voices.
- **Not a physically exact acoustic simulator.** Listener-centric hybrid approximations, budgeted.
- **No replication layer.** Games already replicate gameplay; audio rides on that.
- **No game vocabulary.** The plugin owns `Audio.Event` / `Audio.State` / `Audio.Mix` roots only.
- **No hard dependency on any other dll-* plugin or marketplace content.**

## 6. Success metrics

| Metric | Target |
|---|---|
| Adoption cost | A new event type needs **zero** C++ changes after the first emit call. |
| Core overhead | Emit → render decision < 10 µs (no acoustics) on a desktop CPU. |
| Idle cost | Zero per-frame cost when no events are emitted. |
| Network | Zero bytes replicated by the plugin. |
| Correctness | No double-play under prediction in listen + dedicated PIE. |
| Acoustics (0.4) | ≤ 300 rays/s at default budget; 8-16 high-tier sources. Planning targets — profile and tune. |
| Openness | Every release builds standalone with `RunUAT BuildPlugin` and passes `DLLAudio.*` tests. |

## 7. Constraints

- Unreal Engine 5.7+ (pinned per release in `.uplugin` `EngineVersion`).
- Engine features marked Beta/Experimental (Audio Gameplay Volume, AudioGameplay, Soundscape,
  AudioMotorSim) are wrapped behind optional modules or interfaces, never required by the core.
- Apache-2.0: only content we own. No marketplace assets in the repo.

## 8. Open questions

- Ship MetaSound templates as plugin content (`CanContainContent`) or as a separate sample repo?
- Mix module: one global mix-state stack, or per-local-player for split-screen?
- Acoustic portal authoring: hand-placed actors, generated from level metadata, or both?

Record answers in [DECISIONS.md](DECISIONS.md).
