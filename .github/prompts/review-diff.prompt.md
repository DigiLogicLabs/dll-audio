# Review a diff

Review the current diff (or the given range) against the `AGENTS.md` non-negotiables. Read-only.

Check, citing `file:line`:
1. Core dependency creep (`DLLAudioRuntime.Build.cs`, includes from non-engine modules).
2. Replication added anywhere (RPCs, `Replicated`, `GetLifetimeReplicatedProps`).
3. Tick, per-frame logging, timers left running.
4. Loading/rendering without a local-listener check; sinks skipped on a dedicated server.
5. Game vocabulary (tags, names) leaking into the plugin.
6. Public API removed/renamed without redirects + a CHANGELOG migration note.
7. Missing tooltips/categories on exposed properties; missing visual debug for a new system.
8. Logic without tests; tests that depend on project config.
9. Docs/memory not updated (CHANGELOG, PRD, DECISIONS, MEMORY, TESTING).
10. Licensing: file headers present, no third-party content.

Report findings ranked by severity, each with a concrete failure scenario. Say "none" when clean.
