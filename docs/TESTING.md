# DLL Audio — Testing

**Rule:** never claim "compiles" or "tested" without an artifact (build log line, test report, PIE note).
`UNTESTED` is a valid, honest status.

## 1. Standalone build (every change)

```powershell
& "<UE>\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin `
  "-Plugin=<repo>\DLLAudio.uplugin" "-Package=<temp>\dllaudio_pkg" -TargetPlatforms=Win64 -Rocket
```

Pass = `BUILD SUCCESSFUL`. This builds the plugin in an isolated host project, which proves it has no hidden
dependency on any adopting game. Only one UnrealBuildTool may run per engine install (see MEMORY traps).

## 2. Automation tests

| Test | Covers |
|---|---|
| `DLLAudio.Registry.Specificity` | Most-specific context match, generic fallback, unregister |
| `DLLAudio.Registry.DedupWindow` | Key 0 bypass, window, per-tag keys, pruning |

Run in editor: *Tools → Session Frontend → Automation*, filter `DLLAudio`. Headless:

```powershell
UnrealEditor-Cmd.exe <Project>.uproject -ExecCmds="Automation RunTests DLLAudio; Quit" -unattended -nullrhi -nosplash
```

Add a test for every pure-logic change. Keep logic in plain C++ classes (`FDLLAudioRegistry`,
`FDLLAudioDedupFilter`) so it is testable without a world.

## 3. In-game checklist (PIE)

1. `dll.Audio.Debug 2` — markers + HUD appear only when events fire (no idle cost).
2. Emit an event with a registered definition → green marker, sound heard, `Rendered` increments.
3. Unknown tag → red marker, `No definition` increments, silence.
4. Event beyond `MaxAudibleDistance` → orange, `Culled dist`, no component spawned.
5. 100 one-shots in a frame with `MaxOneShotsPerFrame=32` → 32 rendered, rest `Culled budget`; a
   priority ≥ `AlwaysAdmitPriority` definition still plays.
6. Same `DedupKey` twice within the window → second is purple / `Dedup`.
7. Start a sustained state, push parameters, stop with fade → component cleans up.

## 4. Network matrix

| Mode | Expect |
|---|---|
| Standalone | Renders |
| Listen server | Host renders; clients render independently from their own position |
| Dedicated server | Server: `No listener` for every event, sinks still fire, **no sounds loaded** |
| Split-screen (2-4) | Culling uses the nearest local listener |
| Predicted fire + confirmation | Owner hears one shot (shared DedupKey) |

## 5. Status

| Item | Status |
|---|---|
| Standalone build | 2026-10-15: `RunUAT BuildPlugin -TargetPlatforms=Win64` on UE 5.7 → 3× `Result: Succeeded`, `BUILD SUCCESSFUL`, 0 warnings in plugin sources |
| Automation tests | Written (2); NOT YET RUN — owed in the reference adopter |
| PIE / network matrix | UNTESTED for 0.1.0 |
