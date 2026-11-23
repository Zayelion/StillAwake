# Still Awake

Unity first-playable prototype based on **Still Awake Game Design Document v0.1**, dated 5 November 2026.

The implementation is in `Assets/_Project`. It includes a fixed house circuit, 32 authored anomalies, the two-pass tutorial, movement-based scoring, an eight-return win condition, journal fragments, settings, and a dawn ending. Environment art and synthesized audio are original blockout placeholders.

**Validation status (6 November 2026):** all 38 engine-independent rule checks pass; the core, runtime, editor tooling, and Unity test assemblies compile against the installed Unity assemblies. Unity itself cannot initialize its licensing client in this restricted execution session and crashes before the build. A Windows executable, in-engine test results, visual playtest, and performance measurements are **not yet available**. See `Documentation/Validation.md`.

## Open and play

1. Add this directory as a project in Unity Hub.
2. Open with **Unity 6000.4.11f1** and a working Unity license. The GDD proposed 6.3 LTS; this prototype uses the editor already installed on the development machine.
3. Allow package import to finish. The manifest pins URP 17.4.0, Input System 1.19.0, Unity UI 2.0.0, and Test Framework 1.6.0.
4. Open `Assets/_Project/Scenes/StillAwake.unity` (or **Still Awake > Open game scene**) and press Play. The world is assembled when Play starts; the edit-time scene contains its bootstrap object.
5. Choose New game. The first circuit is clean. The second removes a person from the living-room portrait and teaches the retreat rule.

| Control | Action |
| --- | --- |
| W / A / S / D | Walk |
| Mouse | Look |
| E | Inspect journal / interact with bedroom door / close journal |
| J / K | Previous / next unlocked journal page |
| Esc | Close journal, go back from settings, or pause/resume |

Walk the full circuit when the house is unchanged. When an anomaly is noticed, turn back before the next room doorway and retrace the route. A mistake resets the current streak. Eight consecutive correct rounds lead to dawn. The two successful tutorial rounds count toward the streak.

## Build and validate

Run from PowerShell 7 in the project directory. Close the interactive Unity editor before batch builds. `Build.ps1` is the standard local/CI entry point; see `Documentation/BuildPipeline.md` for setup, stages, and runner configuration.

```powershell
# Fast validation and supplementary compilation (no Unity launch)
.\Build.ps1 -Task Validate
.\Build.ps1 -Task Compile

# Requires Unity licensing and package import to work
.\Build.ps1 -Task Test
.\Build.ps1 -Configuration Development

# Run the development executable printed by the build with --smoke-test
# to perform the optional graphical integration check (separate save file).

# Release build
.\Build.ps1
```

`-UnityPath 'C:\path\to\Editor\Unity.exe'` overrides the editor path. Build timeout defaults to 900 seconds per Unity process and can be adjusted with `-TimeoutSeconds`. A successful process exit alone is insufficient: the pipeline requires passing test XML, a successful Unity report, and a complete fresh player before packaging.

Players go to `Builds/Pipeline/<run-id>/Windows`. Distribute the ZIP under `Artifacts/<run-id>`; it includes the executable, data directory, DLLs, and checksums. Smoke-test screenshots and `smoke-result.txt` are written to `QA` beside the development executable only when it is explicitly run. Per-run logs and pipeline reports are under `Logs/Pipeline/<run-id>`.

## Project structure

| Path | Purpose |
| --- | --- |
| `Assets/_Project/Scripts/Core` | Engine-independent route decisions, streaks, seeded weighted selection |
| `Assets/_Project/Scripts/World` | Fixed 192-metre ring, named props, directional checkpoint planes |
| `Assets/_Project/Scripts/Anomalies` | Baseline snapshots and reusable authored effects |
| `Assets/_Project/Resources/Anomalies` | 32 editable anomaly ScriptableObjects across eight rooms |
| `Assets/_Project/Resources/Story` | Six persistent journal fragments |
| `Assets/_Project/Resources/RoundTuning.asset` | Win target, anomaly probability, speed, reversal tolerance |
| `Assets/_Project/Tests` | Rule matrix and six Unity EditMode acceptance tests |
| `Documentation/Authoring.md` | Content workflow and prototype tradeoffs |

Settings, tutorial completion, lifetime counters, and unlocked story IDs persist to Unity's `Application.persistentDataPath` in `progress.json`. On Windows this normally resolves beneath `%USERPROFILE%\AppData\LocalLow\Toad & Luckdragon\Still Awake`. Continue starts a fresh streak and retains tutorial and story progress; it does not resume a partially walked round. New game replays the tutorial and preserves lifetime story discoveries.

No purchases, external services, accounts, combat systems, or generated third-party artwork are needed by the game. Unity and its packages retain their own license terms.
