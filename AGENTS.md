# Agent guidance

## Project

Still Awake is a native Unreal Engine C++ game for Windows, with Steam distribution planned. Work in this project directory. The active project is `StillAwake.uproject`; its engine association is 5.8. Read `README.md` and `CONTRIBUTING.md` before changing the project.

## Sources of truth

- `Source/StillAwake/`: gameplay, characters, house, HUD, audio, and save handling.
- `Source/StillAwake/Core/`: engine-independent rules and acceptance checks.
- `Content/Data/content.json`: authored anomalies, story, and tuning.
- `Config/`: project, input, maps, and packaging settings. `DefaultGame.ini` owns the game version.
- `Content/StillAwake/`: Unreal assets and the House map.
- `Documentation/Steam-Release.md`: release preparation and SteamPipe usage.

Use current Unreal code and configuration when historical Unity documents disagree. Do not introduce Unity dependencies or migrate engines without a request.

## Working conventions

- Make focused changes and preserve unrelated work. Explain material behavior changes.
- Follow existing Unreal naming and reflection conventions; keep engine-independent rules separate from actors and UI.
- Preserve save compatibility, stable content IDs, and deterministic seeded behavior when changing gameplay. Document intentional migrations.
- Use Unreal Editor or the Unreal Python API for `.uasset` and `.umap` edits; never treat them as text. Preserve authored assets when running preparation tools.
- Keep UTF-8 text intact. Treat player-facing wording as content, not incidental formatting.
- Keep generated output in ignored build/cache folders. Retain source, configuration, content, and release templates in version control.
- Never add credentials, Steam login state, private keys, the Steamworks SDK, or local `steam_appid.txt` files to commits.
- Local builds and edits do not imply publishing. Uploads, branch promotion, and public releases must be within the user's requested scope.

## Verification

Run `python Tools/Verify-Project.py` for content/configuration changes and `pwsh -NoProfile -File Tools/Test-Rules.ps1` for rule changes. For Unreal code changes, compile the affected target and run the `StillAwake` automation group. For visible or packaging changes, verify the packaged player as described in `CONTRIBUTING.md`.

Use existing checks where they cover the change; add meaningful tests for new behavior. Do not claim a build or release succeeded based only on an exit code: verify its expected files and test reports. Report checks actually run, failures, and untested behavior.

`npm run build` runs the root `Build.ps1` pipeline and produces a tested Development player. `npm test` runs checks without launching Unreal; `npm run build:editor` adds editor compilation. Keep stage failure propagation, the project lock, per-run output, and report verification intact. See `Documentation/Local-Build.md` for the current workflow; old build attempts in the runbook are historical evidence.

## Steam preparation

Use Windows Shipping builds for release candidates. Keep `Build/Steam/*.example.vdf` generic; edit ignored `.local.vdf` copies for real uploads. Preview is enabled and branch promotion is unset in the templates. Keep debug symbols privately, verify installation from a Steam test branch, and record the build/version before promoting a release.
