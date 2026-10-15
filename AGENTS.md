# AGENTS.md — DLL Audio

Canonical instructions for AI coding agents (Codex, Claude Code, Copilot, others) and a fast orientation for
human contributors. `CLAUDE.md` and `.github/copilot-instructions.md` point here — **edit this file, not them.**

## What this repo is

An open-source (Apache-2.0) Unreal Engine 5 plugin by Digi Logic Labs: gameplay-tag driven audio event
routing. Gameplay says **what happened**; data decides **how it sounds**; one world subsystem decides
**whether and where it plays** for each local listener. Goals and scope: [docs/PRD.md](docs/PRD.md).

## Read before non-trivial work

1. [docs/README.md](docs/README.md) — knowledge-graph index
2. [docs/PRD.md](docs/PRD.md) — requirements (IDs like `E2`, `A3`) and non-goals
3. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — pipeline and extension points
4. [docs/DECISIONS.md](docs/DECISIONS.md) — the *why*; don't relitigate a recorded decision without new evidence
5. [docs/MEMORY.md](docs/MEMORY.md) — verified facts and traps

## Layout

```
DLLAudio.uplugin
Source/DLLAudioRuntime/          core module (engine + GameplayTags only)
  Public/  DLLAudioTypes.h       FDLLAudioEvent, layers, enums
           DLLAudioDefinition.h  UDLLAudioDefinition, UDLLAudioDefinitionLibrary
           DLLAudioRegistry.h    FDLLAudioRegistry, FDLLAudioDedupFilter (plain C++, unit tested)
           DLLAudioResolver.h    UDLLAudioResolver extension point
           DLLAudioSettings.h    Project Settings > Plugins > DLL Audio
           DLLAudioSubsystem.h   UDLLAudioSubsystem (the entry point)
           DLLAudioTags.h        Audio.Event / Audio.State / Audio.Mix roots
  Private/Tests/                 automation tests (DLLAudio.*)
docs/                            knowledge graph (PRD, architecture, decisions, memory, testing, roadmap)
.github/instructions/            modular rules (tool-agnostic)
.github/prompts/                 reusable task prompts (tool-agnostic)
.claude/commands, .claude/agents Claude Code wrappers around the prompts + a read-only reviewer
```

## Non-negotiables

1. **Core stays engine-only.** `DLLAudioRuntime` may depend on Core, CoreUObject, Engine, GameplayTags,
   AudioExtensions, DeveloperSettings. Anything else (GameplayAbilities, MassEntity, AudioModulation,
   AudioGameplayVolume, third-party SDKs) goes in a **new optional module**. Never depend on another dll-*
   plugin or on a game.
2. **The plugin never replicates.** No RPCs, no replicated properties. Dedup keys handle prediction.
3. **No game vocabulary.** The plugin owns only `Audio.Event`, `Audio.State`, `Audio.Mix`.
4. **No Tick.** Event-driven; timers only for periodic work and only while needed.
5. **Dedicated server renders and loads nothing**, but always notifies sinks.
6. **Visual debug for every system:** settings master toggle + granular toggles, `dll.Audio.*` CVar, world
   shapes, screen HUD. No per-frame logging.
7. **Non-destructive.** Public API (`UFUNCTION`, `UPROPERTY`, public headers) is additive between minors;
   breaking changes need a CHANGELOG migration note and a DECISIONS entry. Use `[CoreRedirects]` for renames.
8. **Testable logic.** Pure logic in plain C++ classes with `DLLAudio.*` automation tests.
9. **Honest status.** Never claim "compiles" or "tested" without an artifact. `UNTESTED` is valid.
10. **Only our content.** Never add marketplace or licensed third-party assets.

## Coding conventions

- Types `UDLLAudio*` / `FDLLAudio*` / `EDLLAudio*`; CVars `dll.Audio.*`; log category `LogDLLAudio`.
- Every file starts with:
  ```
  // Copyright Digi Logic Labs LLC. Licensed under the Apache License, Version 2.0.
  // SPDX-License-Identifier: Apache-2.0
  ```
- Every exposed `UPROPERTY` has a doc comment (it becomes the editor tooltip) and a `Category`.
- Soft references for sounds; hard references only for small config objects.
- Forward-declare in headers; include in `.cpp`.
- More detail: [.github/instructions/cpp.instructions.md](.github/instructions/cpp.instructions.md).

## Build & test

```powershell
# Standalone build (proves no hidden dependency on any game)
& "<UE>\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin "-Plugin=<repo>\DLLAudio.uplugin" "-Package=<temp>\pkg" -TargetPlatforms=Win64 -Rocket
# Tests (inside any project that has the plugin)
UnrealEditor-Cmd.exe <Project>.uproject -ExecCmds="Automation RunTests DLLAudio; Quit" -unattended -nullrhi
```

Only one UnrealBuildTool can run per engine install; a `ConflictingInstance` failure means wait, not retry
with `-NoMutex`. Full matrix: [docs/TESTING.md](docs/TESTING.md).

## Every change leaves Code + Docs + Memory

- Code + tests.
- Docs: the relevant `docs/*.md`, `CHANGELOG.md` under *Unreleased*, PRD requirement status if it changed.
- Memory: a dated line in `docs/MEMORY.md` for non-obvious facts/traps; a `docs/DECISIONS.md` entry for
  choices (append & supersede, never delete).
- Rules: [.github/instructions/ai-memory.instructions.md](.github/instructions/ai-memory.instructions.md).

## Task → prompt map

| Task | Prompt file | Claude Code |
|---|---|---|
| Start a session | `.github/prompts/verify-handoff.prompt.md` | `/verify-handoff` |
| Plan a feature | `.github/prompts/plan-feature.prompt.md` | `/plan-feature` |
| Add an optional integration module | `.github/prompts/add-integration-module.prompt.md` | `/add-integration-module` |
| Review a diff | `.github/prompts/review-diff.prompt.md` | `/review-diff` (or the `dll-audio-reviewer` agent) |
| Cut a release | `.github/prompts/release.prompt.md` | `/release` |
| End a session | `.github/prompts/session-handoff.prompt.md` | `/session-handoff` |

Codex has no repo-shareable slash commands: ask it to follow the prompt file directly.

## Git

- Stage by path; never `git add -A` (adopting projects often have other tools writing nearby).
- Conventional, descriptive commit messages; one concern per commit.
- Don't commit `Binaries/`, `Intermediate/`, packaged output or personal settings
  (`.claude/settings.local.json`).
