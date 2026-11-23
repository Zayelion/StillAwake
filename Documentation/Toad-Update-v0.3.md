# Toad update - Still Awake 0.3.0

Implemented from **Update Report for Still Awake by Toad.docx**, supplied November 9, 2026. The active implementation is native Unreal C++; the original Unity project and the prior cooked v0.2 Windows ZIP are preserved.

## Applied changes

| Request | Implementation |
|---|---|
| Walk 50% faster; hold Shift to run 100% faster | Walk 300 cm/s, run 600 cm/s; either Shift key. No movement while reading or paused. |
| Quiet footsteps, little ambient sound, haunting music | Softer filtered footsteps with attack/release; greatly reduced background noise; original sparse minor melody/pad. Separate SFX, ambience and music controls remain. |
| Remove rain/hum caption | Removed the caption and its obsolete settings toggle. |
| Start outside, facing foyer | Porch spawn, closed front door, table/newspaper to its right. Railings prevent walking into the empty exterior. |
| Click newspaper to read instructions | Large high-contrast overlay with the report's complete wording, including all five rules; click/E/Escape closes it. |
| Dirty white tiles, cream walls, white frames, grey curtains | Tiled floors with muted stains/grout, cream shell, natural-height framed windows and grey curtains. |
| Foyer layout | Hall, right-hand hooks, two cat paintings on the left, table beneath with clock and blue book. |
| Living room layout | Larger square room, brown sofa in the middle facing the TV to the left; a window on either side of the TV. Kitchen entrance is behind the sofa. |
| Kitchen layout | Same footprint as living room; central island/sink/faucet/dishes, dining table on the left, black fridge on the right, back door opposite the entrance. |
| Front/back door choices | Entering the front door never scores. Physically exiting the kitchen means clean; physically exiting the foyer means anomaly. Opening a door alone never scores. Free backtracking and reinspection are allowed. |
| Whole house resets | Transforms, visibility, material tints, doors, TV animation and water reset between rounds. Each new round starts on the porch. |
| Ten-count door number | Both doors show 10 initially, decrease with each correct choice, reset to 10 on either mistake. |
| First loop always safe | New/continued/restarted runs and runs reset by a mistake begin with a clean house. Subsequent rounds use the ten-anomaly library. |
| Ending | At zero, fade fully to black and show the exact requested rest message, followed by a return-to-title button. |

## Anomaly library

| Room | Anomaly | Baseline reset |
|---|---|---|
| Foyer | Both paintings show black cats | One black and one white cat |
| Foyer | Blue book missing | Blue book on end table |
| Foyer | Yellow raincoat on hooks | Empty hooks |
| Living room | TV static with periodic “wake up” | TV off |
| Living room | Small sleeping black cat on sofa | Empty sofa |
| Living room | One window slightly open | Both windows closed |
| Kitchen | Faucet running with audible water | Faucet off |
| Kitchen | Sink empty of dishes | Dishes in sink |
| Kitchen | Fridge door ajar | Fridge closed |
| Kitchen | Magnets on fridge | No magnets |

Each anomalous round applies one change. All ten are eligible after the safe first loop. Seeded selection and recent-anomaly avoidance remain. Existing story fragments/journal/settings remain; v1 save migration preserves settings and cumulative story progress while retiring the old eight-return record/ending.

## Validation and release status

- Static content/configuration validation passed for the ten anomalies and three rooms.
- **38 native C++ checks passed**, including all door/anomaly outcomes, no score before entry, pause, duplicate scoring, ten-count win/reset, lifetime progress, selection coverage and deterministic distribution.
- Unreal **editor module compiled and linked successfully** after the changes.
- The full Shipping pipeline `20261109-201715-0bfbcc69` stopped at content preparation: Unreal exited with an empty output log and no fresh content-ready marker. A read of its MachineId registry key again returned “Requested registry access is not allowed.” This is consistent with the documented assistant-session startup restriction. No registry permissions, engine binaries or security settings were changed.
- The standard compile-only pipeline `20261109-201911-76b49602` passed editor compilation but its game build host exited with code -532462766. A direct UBT invocation generated headers and queued Shipping actions but stalled in UBA; it was stopped. All ten Shipping translation units subsequently compiled and linked successfully using the UBT-generated MSVC response files, with the final changes recompiled and relinked. This is an uncooked executable, not a player distribution. Final compile evidence is in `Validation-v0.3.json`; native results are in `NativeRules-v0.3.txt`.
- **Unreal automation, the new 44-check physical smoke harness, cooking, packaging and visual/audio playtesting have not run successfully for v0.3.** The prior v0.2 package is not this update.

The smoke harness is ready to test all ten apply/reset pairs, porch spawn and real newspaper interaction, walk/run speeds, pause, one clean exit, all ten anomaly exits after visiting the full house, both mistakes/safe resets, zero-count ending and save reload. It uses a separate save slot.

## Next playable build

Run the full Shipping command in [Build-Runbook.md](Build-Runbook.md) from the user's ordinary PowerShell session. The pipeline updates the existing House map's PlayerStart to the porch and requires a fresh v0.3 preparation marker before tests or packaging. Keep the final pipeline report and packaged-player launch verification separate.

The art remains a primitive-mesh prototype and the soundtrack is synthesized. Check readability, cat/raincoat silhouettes, TV message visibility, window/fridge motion, door clearances, sound levels and the ending on a normal graphics/audio launch before distributing the update.
