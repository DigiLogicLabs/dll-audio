# Changelog

All notable changes to DLL Audio. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
versioning: [SemVer](https://semver.org/).

## [Unreleased]
### Added
- Burst merging (`UDLLAudioDefinition::BurstMerge`, `FDLLAudioBurstMerge`, off by default): many occurrences of
  one definition in the same grid cell inside a short window keep `MaxVoicesPerBurst` voices; later ones are
  absorbed into those live voices (volume growth, `BurstCount` parameter) instead of stacking or being dropped by
  the budget. Leading edge (no delay); never merges into a finished voice. New result `EDLLAudioEmitResult::Merged`
  (appended), `Stats.Merged`, HUD line, cyan debug lines, CVar `dll.Audio.BurstMerge` (0 = off everywhere).
  Plain C++ `FDLLAudioBurstTracker` (`DLLAudioBurst.h`). Additive; no migration needed.
- Tests `DLLAudio.Burst.MergeWindowAndCell`, `DLLAudio.Burst.NeverMergesIntoFinishedVoice`,
  `DLLAudio.Burst.MaxVoicesAndVolumeScale`.
- Burst merging keys on the sound as well as the definition: occurrences that share a definition through
  `CallerLayers` but carry different sounds never merge into each other's voices (`FDLLAudioBurstTracker`
  `TryMerge`/`AddVoices` gained a defaulted `Variant` argument - additive). Test `DLLAudio.Burst.DifferentSoundsNeverMerge`.
- `FDLLAudioEvent::CallerLayers`: layers a legacy call site already chose. They seed
  `FDLLAudioResolution::ExtraLayers` and render only when a definition resolves. Additive; no migration needed.
- Test `DLLAudio.Subsystem.CallerLayers`.
### Changed
- Community files: `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1), `SECURITY.md` (private reporting), issue-template
  `config.yml`, and fork → build → test → pull request steps in `CONTRIBUTING.md`.
- Docs: README, `NOTICE` and the `.uplugin` `CreatedByURL` link [digilogiclabs.com](https://digilogiclabs.com); the README license
  section explains Apache-2.0 in plain words (free for any use, including commercial; keep LICENSE/NOTICE).
- `FDLLAudioLayer` is declared before `FDLLAudioEvent` in `DLLAudioTypes.h` (header order only).
### Fixed
- `DLLAudioBurst.h` includes `UObject/WeakObjectPtr.h`: the standalone `BuildPlugin` failed with C2079 (undefined
  `FWeakObjectPtr`); it only compiled inside a game whose other headers pulled the full type in first.

### Verified
- 2026-10-18, UE 5.7, reference adopter PIE: burst merging - one gameplay cue fired 6x in one spot through a
  routed family played 2 voices at volume 1.2; with `dll.Audio.BurstMerge 0` it played 6 at 1.0 (voice count by
  script, not listened to). Earlier PIE (2026-10-17): coin pickups 6 vs 30 voices.
- 2026-10-16, UE 5.7, reference adopter: burst merging - Development editor build `Result: Succeeded`;
  `DLLAudio.*` 6/6 pass headless (3 new `DLLAudio.Burst.*`). PIE: UNTESTED.
- 2026-10-16, UE 5.7, in the reference adopter (MetalLegs): DebugGame and Development editor builds
  `Result: Succeeded`; `DLLAudio.*` 3/3 pass headless. The 0.1.0 tests also passed 2026-10-15 in an
  isolated host project.

## [0.1.0] - 2026-10-15
### Verified
- Standalone `RunUAT BuildPlugin` (UE 5.7, Win64): BUILD SUCCESSFUL. Automation tests and PIE: UNTESTED.
### Added
- `FDLLAudioEvent`, `UDLLAudioDefinition`, `UDLLAudioDefinitionLibrary`, `UDLLAudioSettings`.
- `UDLLAudioSubsystem`: tag registry with context specificity and parent-tag fallback, resolver chain,
  dedup window, listener check, distance pre-cull, per-frame one-shot budget, soft loading, layered
  rendering, sustained states with live parameters, `OnAudioEventEmitted` sinks.
- `UDLLAudioResolver` extension point.
- Native root tags `Audio.Event`, `Audio.State`, `Audio.Mix`.
- Visual debug (`dll.Audio.Debug`) and automation tests (`DLLAudio.Registry.*`).
- AI-ready repo layer: AGENTS.md, CLAUDE.md, Copilot instructions, `.github` rules and prompts, Claude Code
  commands and reviewer agent, docs knowledge graph (PRD, architecture, decisions, memory, testing,
  integration guide, research, AI workflow, dll-* plugin standard), PR/issue templates.
