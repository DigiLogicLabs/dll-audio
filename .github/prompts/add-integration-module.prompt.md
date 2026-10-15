# Add an optional integration module

Use when a feature needs a dependency the core may not take (GameplayAbilities, MassEntity,
AudioModulation, AudioGameplayVolume, AudioMotorSim, a third-party SDK).

1. Name: `DLLAudio<Integration>` (e.g. `DLLAudioGAS`). Folder `Source/DLLAudio<Integration>/`.
2. `Build.cs`: depend on `DLLAudioRuntime` + the integration's modules. Nothing in the core changes.
3. `.uplugin`: add the module (type, loading phase) and the engine plugin it needs under `"Plugins"`.
   Document in README what adopters must enable.
4. Register with the core only through its extension points (resolvers, sinks, subsystem API). If an
   extension point is missing, add it to the core first as a generic, dependency-free interface.
5. Debug + tests + docs: settings section, `dll.Audio.<Integration>.*` CVars, `DLLAudio.<Integration>.*`
   tests, ARCHITECTURE module table, PRD status, DECISIONS entry, CHANGELOG.
6. Verify with the standalone `RunUAT BuildPlugin`.
