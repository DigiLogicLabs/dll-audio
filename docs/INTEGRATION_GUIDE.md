# DLL Audio — Integration guide (adopting it in an existing game)

The goal is **non-destructive, reversible migration**: no existing sound breaks, nothing plays twice, and
every step can be turned off.

## 1. Install and verify

1. Clone into `Plugins/dll-audio`, enable the plugin, add `DLLAudioRuntime` to your `Build.cs`.
2. Run `dll.Audio.Debug 2` in PIE: the HUD should read `Definitions 0` until you register libraries.

## 2. Define your vocabulary (in your game, not the plugin)

Add leaves under the plugin's roots, e.g.:

```ini
; Config/DefaultGameplayTags.ini (your project)
+GameplayTagList=(Tag="Audio.Event.Weapon.Fire")
+GameplayTagList=(Tag="Audio.Event.Weapon.Impact")
+GameplayTagList=(Tag="Audio.Event.Character.Footstep")
+GameplayTagList=(Tag="Audio.State.Vehicle.Engine")
```

Reuse tag families you already have for **context** (surface materials, damage types, movement states) —
don't invent parallel ones.

## 3. Adapter first, call sites later

If your game already has an audio helper, make it forward into `UDLLAudioSubsystem`. That turns on the new
path for every existing caller with no API change. Guard it with a per-feature flag so a family can be
switched back to the legacy path.

```cpp
void UMyAudioLibrary::PlayImpact(const UObject* Ctx, const FHitResult& Hit, USoundBase* AuthoredSound)
{
    if (GetDefault<UMyAudioSettings>()->bRouteImpactsThroughDLLAudio)
    {
        FDLLAudioEvent Event;
        Event.EventTag = MyAudioTags::Impact;
        Event.Location = Hit.ImpactPoint;
        Event.ContextTags.AddTag(ResolveSurfaceTag(Hit)); // your existing surface vocabulary
        UDLLAudioSubsystem::Get(Ctx)->EmitAudioEvent(Event);
        return;
    }
    LegacyPlayImpact(Ctx, Hit, AuthoredSound);
}
```

**Exactly one path per event.** Having both enabled is the most common way to get doubled sounds.

## 4. Keep authored sounds working (resolver)

If gameplay assets already carry sound fields (`ImpactSound` on a weapon config), don't delete them. Write a
`UDLLAudioResolver` that adds the authored sound as a layer — or replaces the definition — and pass the
authored asset through `FDLLAudioEvent::Parameters` or a context tag. Migrate data at your own pace.

## 5. AI hearing

Bind `OnAudioEventEmittedNative` **on the server** and convert events to perception stimuli
(`UAISense_Hearing::ReportNoiseEvent`). Sinks fire even when nothing renders, so a dedicated server hears
everything. Never route AI hearing through the rendered mix.

## 6. Prediction

Give the predicted play and the server-confirmed play the same `DedupKey` (prediction key, shot id). The
second is dropped inside `DedupWindowSeconds`.

## 7. Migrate one family at a time

Suggested order: impacts → footsteps → weapon fire → abilities → vehicles → ambience. For each: flag on,
compare in PIE (listen + dedicated), check the HUD counters, then remove the legacy path in a later release.

## Reference adopter

MetalLegs (Digi Logic Labs) integrates through its existing `UMLAudioLibrary` seam, keeps its
`Environment.Material.*` surface tags as context, and implements its "authored impact + surface layer" rule
as a resolver. Its AI noise reporting stays server-side in the game module.
