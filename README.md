# Still Awake

A first-person psychological horror game built in native Unreal Engine C++. Inspect both floors and mark suspected anomalies with Fix it stickers, then return to bed. Your inspection affects the next sleep sequence. Complete three inspection and sleep cycles to reach the ending.

The current prototype is **0.3.0**, targeting **Windows x64** with **Unreal Engine 5.8**. The existing build notes pin **5.8.2**. Steam distribution is planned; this repository contains preparation templates, not a published Steam release.

## Requirements

- Unreal Engine 5.8.2 with Windows development support.
- Visual Studio C++ build tools and Windows SDK compatible with that engine; install the Game development with C++ workload.
- Python 3.11+ for the local verification and packaging helpers.
- PowerShell 7 for the native rule checks.
- Node.js/npm for the single-command build wrapper. No npm install or downloaded packages are needed.
- Git. A standard `git clone` includes the House map, materials, textures, and source artwork.

## Open and play

1. Open `StillAwake.uproject` in the matching Unreal installation. Generate Visual Studio project files and build `StillAwakeEditor` / Development Editor / Win64 if requested.
2. Open `/Game/StillAwake/Maps/House` and use Play in Editor.
3. Use WASD and mouse to move and look, Shift to run, E or left click to interact, Escape to pause or go back, and J/K to turn journal pages. Walk into a hinged door to push it open. Aim at the door and press E or left click to swing it by hand. In bed, E or left click toggles the lamp.

You can retry a failed sleep sequence with the same inspection result. The game saves settings, journal unlocks, and ending completion. A new run starts with the first inspection.

`Tools/Prepare-Content.py` is an Unreal Editor Python script for preparing the House map/material. It requires the compiled editor module and Unreal's Python plugin; it is not a standalone Python program.

## Build locally

From the project root, run:

```powershell
npm run build
```

This runs source/native/pipeline checks, compiles the editor, prepares content, runs Unreal automation and traversal tests, and packages a **Development** Windows player. It verifies the package and runs its smoke test before printing the executable path. Keep the complete packaged folder together.

| Command | Result |
| --- | --- |
| `npm run build` | Tested Development player for local playtesting |
| `npm run build:shipping` | Same pipeline, with a Shipping player |
| `npm run build:editor` | Checks and editor compilation, without starting Unreal |
| `npm test` | Source, native rule, and pipeline regression checks; no Unreal installation needed |

Output goes to `Builds/<run-id>/Windows/StillAwake.exe`. Logs and `pipeline-result.json` go to `Saved/Pipeline/<run-id>/`. `Builds/latest-development.json` points to the last successful Development player; a failed run never updates it. Builds create no ZIPs and perform no uploads. Launch the printed executable for visual/audio playtesting.

`ProjectVersion` in `Config/DefaultGame.ini` is validated as SemVer. Full builds receive a local sequential build number, recorded in the package and reports and shown on the intro/title screen, such as `v0.3.0 / Build 1`. See [Versions and build numbers](Documentation/Local-Build.md#versions-and-build-numbers) for the counter and release-version policy.

See [Local-Build.md](Documentation/Local-Build.md) for overrides, stages, and troubleshooting. The command fails with a clear error if the Windows session prevents Unreal from accessing its normal registry identity; run it from an ordinary user terminal in that case. Historical diagnostics are in [Build-Runbook.md](Documentation/Build-Runbook.md).

## Project layout

| Path | Purpose |
| --- | --- |
| `StillAwake.uproject` | Unreal project and editor plugin declarations |
| `Source/StillAwake/` | Native game module |
| `Source/StillAwake/Core/` | Rules and engine-independent checks |
| `Content/Data/content.json` | Anomalies, story, and tuning |
| `Content/StillAwake/` | Unreal map and material assets |
| `Config/` | Project, controls, maps, and packaging configuration |
| `Tools/`, `Tests/` | Validation, content preparation, and build helpers |
| `Build/Steam/` | Versioned SteamPipe templates |
| `Builds/`, `Artifacts/`, `Saved/` | Ignored local output and diagnostics |

## Contributing and release preparation

Start with [CONTRIBUTING.md](CONTRIBUTING.md); automated contributors should also read [AGENTS.md](AGENTS.md). The [Steam release guide](Documentation/Steam-Release.md) covers packaging, app/depot configuration, beta testing, and the remaining launch work.

The Steam templates default to preview mode with no automatic branch promotion. Runtime Steam features such as achievements and cloud saves remain separate integration work.

The [0.3 update notes](Documentation/Toad-Update-v0.3.md) describe the current gameplay changes. `Documentation/Authoring.md`, `BuildPipeline.md`, and `Unity-README.md` describe the earlier Unity prototype and are historical references.

## Ownership

No open-source license is supplied. Confirm redistribution rights for code, art, audio, fonts, and third-party dependencies before releasing them. Record required credits and notices as assets are introduced.
