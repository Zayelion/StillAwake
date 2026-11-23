# Content and architecture

> Historical Unity prototype documentation. For the active Unreal project, start with [README.md](../README.md), [CONTRIBUTING.md](../CONTRIBUTING.md), and [Toad-Update-v0.3.md](Toad-Update-v0.3.md). The Unity classes, tools, and route below do not describe the current implementation.

## Fixed route

Foyer > Main hall > Living room > Dining room > Kitchen > Pantry > Study > Rear corridor > Return hall > Foyer.

Eight 24-metre segments form a single-floor ring. At 2 m/s, a centerline circuit takes approximately 96 seconds before inspection stops. Each segment has an entry checkpoint at 4 m and a recognition plane at 10 m. Props are primarily at 11-16 m. The next segment's entry checkpoint closes the retreat window. The final segment's window closes at the foyer finish. A 0.65 m sustained reverse movement commits a retreat; crossing a checkpoint backward also commits it. Facing direction alone does not determine the decision.

`RouteSession` owns scoring and rejects skipped checkpoints, wrong reverse order, duplicate finishes, and attempts to reopen a committed retreat. `GameController` connects actual swept movement to those rules and owns menu/round/result/pause/ending transitions. `RoundSelector` uses an explicit xorshift PRNG; the `--seed 1926` command-line option reproduces selection with the same progression and actions. The game log records the seed, PRNG state, round ordinal, anomaly ID, and result.

## Add an anomaly

1. Create and register a stable prop ID in `HouseBuilder`; preserve an unobstructed center corridor.
2. Create an AnomalyDefinition using **Assets > Create > Still Awake > Anomaly** and place it in `Assets/_Project/Resources/Anomalies`.
3. Set a unique anomaly ID, the exact registered target ID, room ID, route index (1-8), tier (1-3), positive weight, and minimum cumulative progress.
4. Choose Move, Rotate, Scale, Toggle, Text, or Light. Move/rotate/scale values operate in the target's local transform space. Text effects require a TextMesh on the target. Light effects toggle a light-containing object.
5. Verify the target is readable from the approach before the next forward checkpoint. Capture baseline and altered screenshots at FOV 70, 80, and 95 and brightness 0.6, 1, and 1.7.
6. Apply/reset twice, test both travel directions, pause/resume, fail the round, and check the following clean round. The Unity acceptance tests include repeated reset and centerline collision checks.

`AnomalyController` snapshots registered props before the first round, then restores position, rotation, scale, active state, direct TextMesh contents, renderer materials, direct light settings, and direct audio source state before applying at most one effect. Register every independently mutable object. Extend the snapshot if a new effect mutates components outside those fields.

`ContentCatalog` supplies the bootstrap/default definitions. Existing ScriptableObject assets are authoritative and are never overwritten by **Still Awake > Prepare project and content**. Update both the catalog and assets deliberately when changing the default content. `Tools/Create-ProjectAssets.py` is a bootstrap tool that preserves existing assets; it is not a synchronizer. If the intended library size changes, update the explicit content-count assertions.

## Story and settings

StoryFragment assets use stable IDs and cumulative round thresholds. Once unlocked, a fragment remains available after streak loss. The study journal supports J/K page navigation. Adding a fragment does not require changing scoring.

Round tuning lives in `RoundTuning.asset`: eight consecutive wins, 65% anomaly probability, 2 m/s walk speed, and the tutorial portrait anomaly are defaults. Candidate selection excludes the previous three anomaly IDs and previous two rooms when possible, then relaxes constraints for small pools. Difficulty tiers depend on streak; story and ambient progression depend on cumulative rounds.

## Prototype scope and departures

- The pinned editor is the installed Unity 6000.4.11f1 rather than the GDD's proposed 6.3 LTS.
- The route uses original primitive geometry and simple materials. This is a blockout, not the final lived-in house art pass. The clock is a readable digital-style TextMesh placeholder rather than modeled hands.
- Original synthesized sound supplies ambience, music, footsteps, and transitions. The five volume controls use AudioListener/AudioSource levels rather than authored AudioMixer groups. Spatial object sounds and a final mix remain production work.
- Separate room-definition ScriptableObjects are not supplied; room layout and checkpoint definitions are fixed in HouseWorld/HouseBuilder. Anomalies, tuning, and story are data assets.
- The final sequence faces the bedroom door while dawn rises; a furnished bedroom interior and a longer authored ending remain art work.
- Camera motion is a very small optional walk bob and is off by default. There is no sprint, jump, crouch, flashlight, inventory, combat, gamepad support, or required audio-only anomaly.
- The 25-45 minute first-clear target, visual fairness of all anomalies, and 60 FPS at 1080p require actual playtests. Counts and source checks do not establish those acceptance criteria.
