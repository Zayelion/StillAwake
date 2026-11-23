# Contributing to Still Awake

Follow the setup in [README.md](README.md). Use the Unreal version declared by the project and matching build tools; coordinate engine upgrades as their own change.

## Changes

1. Start from the current project state and keep unrelated local changes intact. Once Git is configured, use a short, focused branch.
2. Describe the bug or desired behavior before implementing it. Keep code, content, configuration, and related documentation consistent.
3. Follow existing Unreal C++ conventions: reflected types use the appropriate macros, UObject references use Unreal-supported ownership, and engine-independent rules belong in `Source/StillAwake/Core/`.
4. Edit Unreal assets in the editor. Coordinate changes to the same map or asset since Git cannot merge those files as text.
5. Run checks appropriate to the change and include their results with the review. Use small commits that explain the resulting behavior.

## Content and saves

`Content/Data/content.json` supplies the current anomaly library, story, and tuning. Keep IDs unique and stable, verify targets against `SAHouse.cpp`, and test effect reset between rounds. If an intentional content change alters counts or defaults, update the corresponding checks in `Tools/Verify-Project.py` and `Source/StillAwake/SATests.cpp`.

Preserve existing save data when possible. Include an explicit version/migration decision when changing save schemas, progression, or content IDs. Test both a fresh profile and an existing save. Keep authored assets intact when running preparation scripts.

## Verification

| Change | Checks |
| --- | --- |
| Documentation or templates | Check referenced paths, examples, and placeholders; do not publish while testing documentation |
| Content or configuration | `python Tools/Verify-Project.py`; exercise affected content in Unreal |
| Core rules | `pwsh -NoProfile -File Tools/Test-Rules.ps1` |
| Unreal C++ or assets | Compile the editor target; run the `StillAwake` automation group; playtest the changed behavior |
| Packaging or release | Package Windows Shipping; run `python Tools/Package.py verify-player <packaged-folder>`; test the packaged executable |

For a release playtest, cover the title screen, clean and anomalous rounds, door decisions, reset, ending, pause, audio/settings, and save/reload. A successful compile or static check alone does not establish that the game runs. Preserve useful logs under `Saved/` and report blocked or unrun checks explicitly.

Use `npm test` for source/native/pipeline checks, `npm run build:editor` for those checks plus editor compilation, and `npm run build` for a tested Development player. Use `npm run build:shipping` for release candidates. See [Local-Build.md](Documentation/Local-Build.md) for logs and troubleshooting. Headless smoke tests do not replace visual/audio playtesting.

## Review information

Include the problem, resulting behavior, checks run, known limitations, and screenshots for visible changes. Mention changed assets, engine/configuration changes, and save compatibility when relevant. Bug reports should include the version, reproduction steps, expected/actual behavior, and relevant logs without secrets or personal data.

## Source and release hygiene

- Commit source, authored content, configuration, and `Build/Steam/*.example.vdf`. Keep generated binaries, caches, local packages, and Steam authentication state out of Git.
- Track source artwork and its imported Unreal assets directly in Git. Content preparation uses the PNGs in `Content/Art/`, and the game loads the textures in `Content/StillAwake/Art/`.
- Add asset origin, license, and required credit information with new third-party assets. Do not assign a license to someone else's work.
- Keep `ProjectVersion` in `Config/DefaultGame.ini` and release notes consistent when preparing a release. Tag the tested source version after Git is configured.
- Follow [Steam-Release.md](Documentation/Steam-Release.md) for storefront preparation. Record the tested Steam BuildID and retain the previous working build for rollback.
