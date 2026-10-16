# DLL Audio — Decision log

Append-only. Never delete an entry; supersede it with a newer one that says **Supersedes:**.
Format: `## YYYY-MM-DD — <decision in one line>` then Decision / Reason / Rejected / Refs.

---

## 2026-10-15 — Build a standalone, open-source router; not a game module, not middleware

**Decision:** Gameplay audio routing ships as `dll-audio`, an Apache-2.0 plugin in its own repo, following
the Digi Logic Labs `dll-*` standard. Native UE audio (mixer, MetaSounds, modulation) is the renderer.
**Reason:** Every studio rebuilds this; the reference adopter had 56 scattered call sites and no seam in use.
Native UE covers rendering; what's missing is the gameplay layer above it.
**Rejected:** Wwise/FMOD now (dependency and cost without a proven need); a MetalLegs-only module (not reusable).

## 2026-10-15 — The plugin never replicates

**Decision:** No RPCs, no replicated properties, no network policy on definitions. Each machine emits events
from its own replicated gameplay state; `DedupKey` resolves predicted vs confirmed plays.
**Reason:** Adopters already replicate gameplay (Gameplay Cues, OnRep, multicast). A second audio carrier
causes double-plays and bandwidth with no benefit. Listener-local rendering is required anyway.
**Rejected:** The original plan's router-owned network policy enum. An optional tiny event channel for sounds
with no carrier may be added later as a separate module.

## 2026-10-15 — Core depends on engine + GameplayTags only; integrations are optional modules

**Decision:** `DLLAudioRuntime` depends on Core, CoreUObject, Engine, GameplayTags, AudioExtensions,
DeveloperSettings. GAS, Mass, modulation and acoustics come as separate modules. Integrations with specific
third-party or sister plugins (a crowd plugin, a voxel plugin) live in the adopting game.
**Reason:** Adopters must not inherit dependencies they don't use; Beta engine APIs must break one module,
not the plugin. dll-* plugins never depend on each other.

## 2026-10-15 — Wrap engine Beta systems, don't re-implement them

**Decision:** Acoustic zones build on Audio Gameplay Volumes; modular emitter slots on `AudioComponentGroup`;
vehicle synthesis optionally on AudioMotorSim. Our code adds what the engine lacks (portal graph, gameplay
budgets, resolver chain).
**Rejected:** A parallel custom reverb-zone system (the original plan's `FAcousticZone` reverb fields).

## 2026-10-15 — Game precedence rules are resolvers, not core features

**Decision:** The registry picks one definition; `UDLLAudioResolver` subclasses may replace it or add layers.
**Reason:** The reference adopter's rule ("authored impact always plays; surface layer under it") is
game-specific. As a resolver it stays one rule, not a third resolution system.

## 2026-10-15 — Late one-shots are dropped, not delayed

**Decision:** An unloaded one-shot requests an async load and is skipped (`PendingLoad`). Sustained states
load synchronously. `bPreloadSounds` (default on) makes the first occurrence audible.
**Reason:** A gunshot heard 200 ms late is worse than one missed shot. Loops are long-lived; a hitch at start
is acceptable.

## 2026-10-15 — Event struct: no `SurfaceHardness`; add Instigator, attachment, DedupKey, Parameters

**Decision:** Surface identity travels as a context tag. `FDLLAudioEvent` is the local transient form
(raw pointers, never replicated).
**Rejected:** The breakdown's `SurfaceHardness` float (duplicates the material tag) and `UObject* Source`.

## 2026-10-15 — Apache-2.0 for every dll-* plugin

**Decision:** Apache-2.0 for dll-audio and dll-mass-units (formerly ue5-mass-units-plugin, which had no license).
**Reason:** Permissive like MIT, plus an explicit patent grant and contribution terms (section 5), which matters
for a company-maintained project accepting outside PRs without a CLA.
**Refs:** [DLL_PLUGIN_STANDARD.md](DLL_PLUGIN_STANDARD.md)

## 2026-10-16 — Caller layers: a migration aid, never a bypass

**Decision:** `FDLLAudioEvent::CallerLayers` lets a call site pass sounds it already resolved. They seed the
resolution's `ExtraLayers`; they never render without a resolved definition.
**Reason:** Adopters already hold sound fields on gameplay data (weapon configs). Re-authoring every one as a
definition before adopting is a migration wall. Keeping rendering gated on a definition keeps data (attenuation,
concurrency, budget priority) in charge of how anything plays.
**Rejected:** Passing the asset as an object `FAudioParameter` (it would leak into every sound's parameter set),
or a hidden resolver side channel (invisible in the event and the debug view).
