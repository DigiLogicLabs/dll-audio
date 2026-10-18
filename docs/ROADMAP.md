# DLL Audio — Roadmap

Each phase is shippable on its own and gated on the previous one being used in a real game.

## 0.1 — Foundation (this release)
- [x] Semantic event struct, definitions, libraries, project settings
- [x] World subsystem: registry (specificity + parent fallback), resolver chain, dedup, listener check,
      distance pre-cull, per-frame budget, soft loading, layered rendering, sustained states
- [x] Sinks for AI hearing / telemetry / accessibility
- [x] Visual debug (`dll.Audio.Debug`), automation tests for registry + dedup

## 0.2 — Authoring & mix
- [ ] `DLLAudioEditor`: definition validation (missing sounds, unregistered tags), create-from-sound action
- [ ] MetaSound templates: one-shot, sustained loop, layered event (shipped as optional plugin content)
- [ ] `DLLAudioMix`: `Audio.Mix.*` states → Audio Modulation buses; submix/sound-class template
- [ ] Event-to-audible latency + per-tag counters in the debug HUD

## 0.3 — Integrations
- [ ] `DLLAudioGAS`: Gameplay Cue notify that emits audio events; prediction-key dedup helper
- [ ] Vehicle telemetry interface (RPM, throttle, load, speed, surface) → sustained-state parameters;
      optional AudioMotorSim backend
- [ ] Logical emitter slots for modular characters (AudioComponentGroup backend)

## 0.4 — Acoustics
- [ ] `DLLAudioAcoustics`: acoustic zones on Audio Gameplay Volumes, portal graph with openness/transmission,
      weighted routing, apparent-source-at-portal
- [ ] Batched async direct-path occlusion on a dedicated trace channel, per-tier rates, temporal smoothing,
      result caching, global ray budget
- [ ] Acoustic materials (3-band absorption/transmission) keyed by gameplay tag
- [ ] Dirty-region invalidation for destructible / voxel worlds
- [ ] Provider interface; Steam Audio provider evaluated separately

## 0.5 — Scale
- [x] One-shot burst merging in the core (`UDLLAudioDefinition::BurstMerge`), built 2026-10-16 ahead of 0.5;
      PIE untested
- [ ] `DLLAudioMass`: aggregate crowd emitters (size, speed, aggression, % attacking) with near-field
      hand-off to individual emitters
- [ ] Component pooling, emitter LOD, perceptual priority weighting
- [ ] Benchmark map + Audio Insights / Unreal Insights trace markers

## 1.0
- [ ] API freeze, full docs, sample project
