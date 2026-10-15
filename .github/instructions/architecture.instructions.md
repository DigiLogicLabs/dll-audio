---
applyTo: "**"
---
# Architecture rules (DLL Audio)

Four concerns, never tightly coupled: **generation** (events/definitions), **propagation** (acoustics),
**perception** (listeners; AI hearing via sinks), **mixing** (budgets, mix states).

- The engine renders, virtualizes and manages concurrency. We add gameplay-informed decisions above it —
  never re-implement voice management.
- Engine Beta/Experimental features (Audio Gameplay Volume, AudioGameplay, Soundscape, AudioMotorSim) are
  wrapped behind an optional module or a provider interface.
- Game-specific rules are **resolvers** or **sinks**, not core branches.
- Integrations with a specific third-party/sister plugin belong in the adopting game or a bridge module.
- New subsystems follow the pipeline in `docs/ARCHITECTURE.md`; update that doc when the pipeline changes.
- Check `docs/DECISIONS.md` before proposing an alternative to something already decided.
