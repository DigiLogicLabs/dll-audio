# Plan a feature

Input: a feature request.

1. Map it to `docs/PRD.md` requirement IDs (add a new ID if none fits; say so).
2. Read `docs/ARCHITECTURE.md` and `docs/DECISIONS.md`. Name any decision the plan touches.
3. Decide the home: core (`DLLAudioRuntime`, engine-only deps) or a new/existing optional module. Justify.
4. Design: public types + signatures, data assets/settings, extension points, debug visualisation,
   dedicated-server behaviour, split-screen behaviour, per-frame cost when idle (must be zero).
5. Tests: which `DLLAudio.*` automation tests and which manual checks in `docs/TESTING.md`.
6. Docs + memory: which files change (CHANGELOG, PRD status, ARCHITECTURE, DECISIONS, MEMORY).
7. Risks, and what would make you choose differently.

Output a plan; don't write code until it is approved.
