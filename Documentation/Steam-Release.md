# Steam release preparation

This is preparation for a future **Windows x64** release. No Still Awake AppID or depot has been configured here, no Steam upload has been performed, and runtime Steam integration is not enabled. The project currently declares version `0.3.0` in `Config/DefaultGame.ini`.

## Prepare the game

1. Run `npm run build:shipping` using the [local pipeline](Local-Build.md) for a tested Shipping package. Use its printed output path as the SteamPipe content root. The remaining steps describe the equivalent manual editor workflow.
2. In Unreal Editor, select **Shipping** for Windows packaging. `DefaultGame.ini` currently defaults to Development; explicitly change the configuration for the release candidate. Preserve the House map, runtime-loaded shapes/fonts, and the staged `Content/Data` directory.
3. Package into a new, empty output directory under `Builds/`. The example below assumes the finished player is in `Builds/Steam/Windows`. Use the actual output location if different.
4. Verify the package, then launch `StillAwake.exe` and exercise the release playtest in [CONTRIBUTING.md](../CONTRIBUTING.md).

```powershell
python .\Tools\Package.py verify-player '.\Builds\Steam\Windows'
```

Retain the complete package and private symbols with the tested version and source revision. Keep all shipped runtime dependencies and cooked containers together. This guide does not establish a successful build; packaging and installation still require verification on the release machine.

## Prepare Steamworks

- Obtain the Still Awake AppID and Windows depot from the project's Steamworks account. Add the depot to the appropriate testing/ownership packages.
- Configure the Windows launch option as `StillAwake.exe`, relative to the package root, and verify it after installation.
- Configure the required runtimes under Installation > Redistributables. Unreal's `bIncludePrerequisites=True` stages an installer; it does not configure Steam to run it. Test on a clean Windows machine without development tools.
- Create a test beta branch and arrange access for the test accounts.

## SteamPipe templates

Copy `Build/Steam/app_build.example.vdf` and `depot_build_windows.example.vdf` to matching `.local.vdf` names in the same folder. Local copies are ignored by Git. Replace `YOUR_APP_ID`, `YOUR_WINDOWS_DEPOT_ID`, and `YOUR_BUILD_DESCRIPTION`. Confirm `ContentRoot` points to the complete package; the relative default resolves to `Builds/Steam/Windows`.

`Preview=1` produces a manifest without uploading. `SetLive` is empty. Run SteamCMD from your separately installed Steamworks SDK, using an interactive login or its cached credentials; keep passwords out of commands and source files:

```powershell
$steamCmd = 'C:\SteamworksSDK\tools\ContentBuilder\builder\steamcmd.exe'
$appBuild = (Resolve-Path '.\Build\Steam\app_build.local.vdf').Path
& $steamCmd +login YOUR_BUILD_ACCOUNT +run_app_build $appBuild +quit
```

Review the manifest in `Saved/SteamPipe`: include runtime files and exclude symbols, logs, and `steam_appid.txt`. For an intended upload, set the local copy's `Preview` to `0`. After upload, assign the build to the test branch in Steamworks and install through Steam. Record the BuildID; promote only the tested candidate.

## Optional runtime Steam features

Steamworks API integration is optional for distribution. Add Unreal's OnlineSubsystemSteam only when implementing features such as achievements, Steam identity, or cloud saves. Follow Epic's configuration guide for the installed engine version and validate the required runtime libraries. Do not ship a development `steam_appid.txt` or use the shared test AppID as this game's identity.

Cloud saves require a deliberate save-path and conflict-handling decision. Determine the actual packaged save location and test synchronization before enabling or advertising the feature. Likewise, verify controller support and Steam Deck behavior before claiming them.

## Release checklist

- [ ] App/depot, launch option, ownership packages, and test branch configured.
- [ ] Shipping build passes automated checks and a packaged playtest.
- [ ] Fresh Steam installation, update from the prior version, settings, saves, and relevant offline behavior tested.
- [ ] Required prerequisites install correctly on a clean Windows machine.
- [ ] Store description, screenshots, capsule/library art, trailer, languages, supported features, and system requirements match the actual build.
- [ ] Content survey and applicable disclosures completed; asset rights, credits, and third-party notices reviewed.
- [ ] Store page and near-final build submitted for Valve review; current release requirements checked in Steamworks.
- [ ] Version, source revision/tag, test results, Steam BuildID, and rollback build recorded.

## References

Checked November 10, 2026; recheck platform requirements when preparing the actual release.

- [Epic: packaging projects](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project)
- [Epic: Online Subsystem Steam](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-steam-interface-in-unreal-engine)
- [Valve: uploading with SteamPipe](https://partner.steamgames.com/doc/sdk/uploading)
- [Valve: Steamworks API overview](https://partner.steamgames.com/doc/sdk/api)
- [Valve: depots](https://partner.steamgames.com/doc/store/application/depots)
- [Valve: beta branches](https://partner.steamgames.com/doc/store/application/branches)
- [Valve: common redistributables](https://partner.steamgames.com/doc/features/common_redist)
- [Valve: review process](https://partner.steamgames.com/doc/store/review_process)
