---
applyTo: "Source/**/Tests/**"
---
# Testing rules (DLL Audio)

- Test names: `DLLAudio.<Area>.<Behaviour>`; flags `EditorContext | EngineFilter`.
- Tests must run in **any** project: use only the plugin's own root tags and transient objects.
- Clean up anything rooted or spawned.
- Prefer testing plain C++ classes; world-based tests only when the behaviour needs a world.
- A test that passes before the fix proves nothing — when fixing a bug, check the test fails without it.
- Report results with the exact command and the pass/fail counts.
