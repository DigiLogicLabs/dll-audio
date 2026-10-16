---
applyTo: "Source/**/*.{h,cpp,cs}"
---
# C++ rules (DLL Audio)

Shared rules for every dll-* plugin: [docs/DLL_PLUGIN_STANDARD.md](../../docs/DLL_PLUGIN_STANDARD.md#coding-rules-shared-by-every-dll--plugin).
The rules below are specific to this plugin.

- **Dependencies:** `DLLAudioRuntime.Build.cs` is a contract. Adding a module dependency there is a
  breaking architectural change — put it in a new optional module instead (see
  `.github/prompts/add-integration-module.prompt.md`).
- **Naming:** `UDLLAudio*`, `FDLLAudio*`, `EDLLAudio*`, `IDLLAudio*`; CVars `dll.Audio.*`; `LogDLLAudio`.
- **Header hygiene:** forward-declare; include in `.cpp`; `DLLAUDIORUNTIME_API` on exported types.
- **UPROPERTY:** doc comment above (becomes the tooltip), `Category`, `ClampMin/Max`/`Units` where numeric,
  `EditCondition` for debug sub-toggles. Soft pointers for sounds.
- **No Tick, no per-frame logging.** Timers only while needed; clear them in `Deinitialize`/`EndPlay`.
- **Net mode:** anything that loads or renders checks for a local listener first; sinks always fire.
- **Parameters before Play:** create components with `FAudioDevice::CreateComponent(..., bPlay=false)`,
  set parameters, then `Play()`.
- **Lifetime:** weak pointers to components and world objects held across frames; `UPROPERTY` for owned UObjects.
- **Testability:** decision logic in plain C++ classes, covered by `DLLAudio.*` tests.
- **Public API is additive** between minor versions. Renames use `[CoreRedirects]` + CHANGELOG migration note.
