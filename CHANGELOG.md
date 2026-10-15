# Changelog

All notable changes to DLL Audio. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
versioning: [SemVer](https://semver.org/).

## [Unreleased]

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
