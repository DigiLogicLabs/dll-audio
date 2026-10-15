# DLL Audio — Project memory

Short, dated, verified facts and traps that the code doesn't make obvious. One line each, newest at the
bottom. Mark unverified items **UNVERIFIED**. Promote anything that becomes a rule into
[DECISIONS.md](DECISIONS.md). Rules for writing here: [../.github/instructions/ai-memory.instructions.md](../.github/instructions/ai-memory.instructions.md).

## Facts

- 2026-10-15 — 0.1.0 scaffolded. Runtime module only; `EnabledByDefault=false`, `CanContainContent=false`.
- 2026-10-15 — UE 5.7 `FAudioDevice::CreateComponent` with `bPlay=false` lets parameters be set before
  `Play()`; `UGameplayStatics::SpawnSoundAtLocation` plays immediately, so MetaSounds could miss initial
  parameters. The subsystem uses the former.
- 2026-10-15 — UE 5.7 `ISoundHandleSystem` (componentless playback) exposes only transform, play and stop —
  no parameters, attenuation override or filter control. Usable for cheap one-shots only.
- 2026-10-15 — Steam Audio is not bundled with UE 5.7 (no folder under `Engine/Plugins`). UNVERIFIED: 5.7
  support in Valve's own releases.
- 2026-10-15 — Resonance Audio ships in 5.7 marked Beta and is enabled by default, but upstream is not
  maintained. Don't build on it.
- 2026-10-15 — Automation tests run green in an isolated content-only host project using the packaged
  plugin binaries (TESTING §2a). That is the adopter-independent way to test; no game build needed.

## Traps

- **Global UBT mutex.** `RunUAT BuildPlugin` fails with `ConflictingInstance` (exit 10, misleadingly
  reported as `Error_SDKNotFound`) while any other UnrealBuildTool runs for the same engine install. Wait
  for it; don't pass `-NoMutex`.
- **260-char paths.** `BuildPlugin -Package=` under a deep temp folder fails after UHT with
  `OtherCompilationError` and "action paths are longer than 260 characters". Package to a short path
  (e.g. `D:\build\dllaudio_pkg`).
- **Native tags in tests.** Automation tests use only the plugin's root tags (`Audio.Event/State/Mix`) so they
  run in any project. Child tags need project config and would make tests adopter-specific.
- **Libraries must be cooked.** Settings hold soft refs; add the `DLLAudioDefinitionLibrary` primary asset
  type or Always Cook, or packaged builds register nothing (a warning is logged).
