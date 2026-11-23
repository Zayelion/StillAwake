# Local Unreal build pipeline

From the project root:

```powershell
npm run build
```

This produces a **Development** Windows x64 player for dev testing. No npm installation step is needed: the Node wrapper uses only built-in modules and starts PowerShell 7. Requirements are Unreal Engine **5.8.2**, Visual Studio C++ build tools, the Windows SDK, and Python 3.11+. Close Unreal Editor in the current Windows session before building; the pipeline does not close it for you.

## Commands

| Command | Work performed |
| --- | --- |
| `npm run build` | Full Development pipeline |
| `npm run build:shipping` | Full pipeline with Shipping packaging |
| `npm run build:editor` | Validation and editor compilation; no editor launch or player packaging |
| `npm test` | Source validation, native C++ rules, and build pipeline regression checks |
| `pwsh -NoProfile -File .\Build.ps1 -Task Test` | Validation, editor compilation, preparation, automation, and editor traversal tests |

The PowerShell equivalent of the default command is `pwsh -NoProfile -File .\Build.ps1`. `-Task Validate`, `-Task Compile`, and `-Configuration Shipping` correspond to the other npm commands. `-Task Editor` aliases compile-only work. The historical `-SkipZip` flag is accepted for compatibility; all tasks now produce local folders and never create ZIPs.

## Full build stages

1. Validate the authored data, staging configuration, and required source files.
2. Compile and run the 38 native rule checks.
3. Run version, counter, package, and build regression checks, including nonzero exits, missing outputs, timeout handling, and incomplete test reports.
4. Check Unreal's normal Windows registry access and the installed engine version.
5. Compile `StillAwakeEditor` using the engine's bundled .NET/UnrealBuildTool.
6. Run `Tools/Prepare-Content.py` inside Unreal and require a fresh content-ready log marker.
7. Run the `StillAwake` automation group and verify the rules, authored content, and build identity tests in its JSON report.
8. Run the editor's headless physical traversal smoke test and require at least 44 passing checks.
9. Build, cook, stage, and archive the Windows player using AutomationTool.
10. Verify required runtime files, then run the packaged runtime's headless smoke test.

The pipeline exits nonzero on failure and never declares an unverified player successful. The smoke tests use the separate `StillAwakeSmoke` save slot. They do not establish visual quality, audio correctness, Steam integration, or human playtest acceptance.

## Outputs

- `Builds/<run-id>/Windows/StillAwake.exe`: launcher printed only after the full build passes. Keep its adjacent folders/files.
- `Saved/Pipeline/<run-id>/pipeline-result.json`: status, version/configuration, stage exit codes, durations, errors, and successful output paths.
- The same log directory contains per-stage stdout/stderr, Unreal logs, automation JSON, and smoke results.
- `Builds/latest-development.json` or `latest-shipping.json`: pointer to the last verified player for that configuration. Failed runs do not replace it.
- `Builds/<run-id>/Windows/StillAwake/build-info.json`: the packaged version, build number, full SemVer build identity, run ID, and configuration. Keep this file with the player.

## Versions and build numbers

`Config/DefaultGame.ini` owns the release version through `ProjectVersion`. It must be valid [Semantic Versioning 2.0.0](https://semver.org/), such as `0.3.0` or `0.4.0-rc.1`. Change it intentionally for a release; rebuilding does not bump the release version.

A full build reserves the next local number in ignored `Saved/BuildNumber.txt` while holding the project build lock. The first numbered build is 1. Failed full builds consume their number, so a later attempt cannot reuse that identity. Validation, compile-only, and test tasks do not advance the counter. This sequence belongs to the local workspace; retain the counter when cleaning build caches. The unique run ID also identifies the build in reports.

The full identity is `0.3.0+build.1`. An existing prerelease or metadata suffix is preserved, for example `0.4.0-rc.1+test.build.2`. The pipeline writes the identity into the packaged player, its result report, and the latest-success pointer. The packaged smoke test must report the same identity before the build can pass.

The intro/title footer shows `v0.3.0 / Build 1`. Editor or manual runs without a package stamp show the configured version with `Build local`.

For example, launch the last successfully tested Development player from your terminal:

```powershell
$build = Get-Content .\Builds\latest-development.json -Raw | ConvertFrom-Json
& $build.executable
```

Run directories are unique; previous builds remain available. The OS-held `Saved/build.lock` prevents overlapping pipelines and releases when the process ends. The file remaining on disk does not mean the project is locked. No build task uploads to Steam or publishes a release.

## Tool paths and troubleshooting

The wrapper checks `STILL_AWAKE_PWSH`, the known local runtime location, the standard PowerShell 7 installation, and PATH. Override tool locations before running the same command when needed:

```powershell
$env:STILL_AWAKE_PWSH = 'C:\Program Files\PowerShell\7\pwsh.exe'
$env:STILL_AWAKE_UNREAL_ENGINE = 'D:\Epic Games\UE_5.8'
npm run build
```

Only set these to installed locations. Direct PowerShell invocation also accepts `-EngineRoot`, `-Python`, and `-TimeoutSeconds` (default 3600 per stage). Unreal's bundled .NET is selected explicitly. .NET tools use native argument lists; Unreal executables receive the quoted option values required by Unreal's raw command-line parser. Stage logs remain readable during long builds; the console prints periodic progress.

If `unreal-environment` reports registry access denied, run the command from an ordinary Windows terminal under the account that normally runs Unreal. The check reads/opens handles only and does not alter registry values, permissions, or engine files. `npm test` and `npm run build:editor` remain available for independent verification. A successful compile is not a successful packaged build.

For a failed stage, start with `pipeline-result.json`, then its named `.log`/`.stderr.log` and engine report. Fix the reported problem and rerun. The GitHub workflow checks out the source with Git and runs the same build script. Packaging requires a configured Unreal Windows runner.

## Verification on November 10, 2026

Source validation, 38 native rule checks, 24 pipeline regression checks, and editor compilation passed. AutomationTool's help command also accepted the nested paths with spaces. Concurrent-build rejection and invalid npm-wrapper options were verified.

The default `npm run build` selected Development, passed its validation stages, then correctly exited with code 1 at `unreal-environment`: this assistant-managed session cannot access `HKCU:\Software\Epic Games\Unreal Engine\Identifiers`. Content preparation, engine tests, cooking, packaging, and packaged smoke testing therefore remain unverified for this restored pipeline. No successful-player pointer was created. Run the same command from the normal Unreal user's Windows terminal to complete those stages.
