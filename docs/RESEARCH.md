# DLL Audio — Research & design lessons

Distilled from the Digi Logic Labs technical breakdown "Building a AAA Spatial Audio System in UE5"
(2026-10) and the review that turned it into this plugin. These are **design lessons**, not claims about how
any studio implemented its game.

## Lessons from reference games

| Game | Strength | What we adopt |
|---|---|---|
| ARC Raiders | Environmental believability, threat localisation without UI | Field-recorded sources, distinct machine signatures, convincing distant combat, environment-aware audio |
| Alien: Isolation | Threats beyond line of sight | Occluded movement, room-to-room propagation, events that also feed AI perception |
| Returnal | High-density combat clarity | Projectile flybys, priority-aware mixing, distinct ability signatures |
| Dead Space | Material acoustics and tension | Material resonances, mechanical ambience, adaptive tension layers |
| Alan Wake 2 | Cinematic transitions | State-driven ambient beds, threat-based mixing, meaningful dynamic range |
| Helldivers 2 | Readable large battles | Near/far layers, battlefield prioritisation, large-event tails, directional warnings |
| INSIDE | Sparse spatial storytelling | Silence as a tool, detail Foley, quiet spaces |
| Naughty Dog titles | Animation/cinematic integration | Animation-driven Foley, per-character materials, dialogue intelligibility |

## The five acoustic effects

1. **Distance attenuation** — engine attenuation curves + listener focus (already native).
2. **Occlusion / transmission** — walls attenuate, more at high frequencies; concrete ≠ wood panel.
3. **Diffraction / pathing** — sound goes around corners and through doors; one ray can't model it.
4. **Early reflections** — apparent size, geometry and material of the space.
5. **Late reverberation** — tunnel vs hangar vs forest vs cave.

## Chosen approach: listener-centric hybrid solver

Don't ray-trace every sound. Per emitter-listener pair, pick a tier:

| Tier | Example rays | Rate | For |
|---|---|---|---|
| High | 3-8 sample points | 10-20 Hz | Threats, bosses, nearby combat |
| Medium | 1-3 | 2-5 Hz | Ordinary enemies, world objects |
| Low | 0-1 / cached zone relation | ≤1 Hz | Distant / ambient sources |

*Illustrative starting budgets, not industry constants. Profile on target hardware.*

- **Direct path:** async, batched traces against a simplified acoustic channel; multiple sample points on
  large sources; cached + temporally smoothed. Never trace from the audio render thread.
- **Rooms & portals:** zones with reverb character; portals with openness and transmission. Route by
  weighted graph search; the doorway becomes the apparent source of indirect sound; crossfade direct vs
  propagated. A closed door still transmits bass — closing it must not silence a loud source.
- **Materials:** low/mid/high absorption + transmission + scattering, authored and calibrated, keyed by
  gameplay tag.
- **Reflections:** baseline = zone presets + shared submix processing. Steam Audio evaluated separately as
  a provider (not bundled with UE 5.7; Apache-2.0; validate per engine version and for dynamic geometry).
- **Never** run a custom occlusion and a middleware occlusion on the same source.

## Networking model

Server decides **what happened**; each client decides **how it sounds** from its own position.

| Sound | Carrier |
|---|---|
| Weapon fire | Local prediction + game's replicated fire event; dedup key |
| Explosion | Server-authoritative gameplay event; rendered per listener |
| Footsteps | Replicated movement state / animation on each machine |
| Ambient machinery | Replicated state changes, never continuous playback |
| Vehicle engine | Existing replicated movement/vehicle state, interpolated locally |
| Mass crowd | Replicated aggregate state; synthesised locally |
| Critical threat cue | Reliable state where loss would mislead the player |

Network relevancy for competitive play should account for **audibility**, not only visibility.

## Mass crowds

Aggregate, don't multiply: group agents by spatial cell, activity and type into a small number of
aggregate emitters with parameters (crowd size, speed, aggression, dominant size, % attacking), and hand off
to individual Foley only near the listener. Mirrors Mass representation/simulation LOD.

## Vehicles

Audio follows **simulation telemetry** (RPM, throttle, speed, gear, surface, suspension, damage, boost),
not input buttons. One telemetry interface; per-vehicle MetaSound presets. Interior/exterior listening mode
by seat and camera, not a master-volume dip.

## Voxel / destructible worlds

Three layers: static (precomputed zones), dynamic (simplified chunk collision + material for occlusion),
invalidation (mark chunks/portals/routes dirty, recompute async under a budget, blend old → new).

## Mixing

Submix tree (Music, Dialogue, UI, Player{Weapons, Abilities, Foley}, Enemies, Vehicles,
Environment{Ambience, Weather}, Reverb returns) + `Audio.Mix.*` states on Audio Modulation buses. Subtle,
intentional ducking — not blanket volume drops.

Priority sketch: `P = w_d·D + w_t·T + w_g·G + w_o·O` (distance, tactical, gameplay, ownership; normalised,
weights tuned empirically).

## Content pipeline (for adopters)

Lossless masters (24-bit/48 kHz+) kept outside the engine; mono for point sources, stereo/multichannel for
beds. Preserve provenance, editing history and variation groups. A DAW such as REAPER for cleanup,
layering and variation sets.

## Engine feature audit (UE 5.7.2, verified in the engine install)

| Feature | Status | Our use |
|---|---|---|
| MetaSound, Audio Mixer, Submixes, Attenuation, Concurrency | Stable | Renderer — always |
| AudioModulation | Plugin | `DLLAudioMix` |
| AudioGameplayVolume | Beta | Acoustic zones provider |
| AudioGameplay (`AudioComponentGroup`, `SoundHandleSubsystem`) | Beta / new | Emitter slots; componentless one-shots (no param control — not for solver-driven sounds) |
| AudioMotorSim | Experimental | Optional vehicle backend |
| Soundscape | Beta | Optional procedural ambience |
| Audio Insights | Beta, on by default | Profiling |
| Attenuation `bEnableOcclusion` | Stable, single ray | Replaced per sound by our solver; never both |
| Resonance Audio | Beta, unmaintained upstream | Not used |
| Steam Audio | Not bundled | Optional provider, evaluated separately |
