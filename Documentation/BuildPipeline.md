# Standard build pipeline

> Historical Unity prototype documentation. The active game is Unreal Engine; use [Local-Build.md](Local-Build.md) for the current pipeline and [Steam-Release.md](Steam-Release.md) for release preparation. The Unity stages and options below do not apply to the restored Unreal build script.

Run `pwsh -File .\Build.ps1` from the project directory for a tested, packaged Windows release build. The same entry point is used locally and by GitHub Actions. This creates files only; it does not publish a release or upload to a storefront.

## Commands

| Command | Stages |
| --- | --- |
| `pwsh -File .\Build.ps1` | Rule checks, source checks, packaging tests, script syntax, Unity EditMode tests, Windows release build, ZIP and SHA-256 |
| `pwsh -File .\Build.ps1 -Configuration Development` | Same stages, with Unity development/smoke-test tools enabled |
| `pwsh -File .\Build.ps1 -Task Validate` | Rules, source integrity, packaging failure tests, script syntax; no Unity launch |
| `pwsh -File .\Build.ps1 -Task Compile` | Validation plus supplementary compilation against installed Unity assemblies; no Unity launch |
| `pwsh -File .\Build.ps1 -Task Test` | Validation plus Unity EditMode tests |
| `pwsh -File .\Build.ps1 -Task Source` | Validation plus a portable source ZIP |

Prerequisites: Windows x64, PowerShell 7, Python 3.11 or later, and Unity 6000.4.11f1 with Windows Mono build support. The Test/Build tasks require Unity licensing to work under the account running the pipeline. `Validate` and `Source` do not require an installed editor; `Compile` requires its reference assemblies but does not start Unity.

The editor version is read from `ProjectSettings/ProjectVersion.txt`. The default path is the matching Unity Hub installation. Override it with `-UnityPath 'C:\path\to\Editor\Unity.exe'` or the `STILL_AWAKE_UNITY_EDITOR` environment variable. The executable's reported version is checked before launch. `-Python` accepts an alternate Python executable. The product version defaults to `bundleVersion` in ProjectSettings; use `-Version 0.1.1` for a specific build. `-TimeoutSeconds 1200` changes the per-Unity-process timeout.

## Outputs and failure behavior

Every run gets a timestamp and random ID. The pipeline takes an exclusive project lock, checks for an open Unity editor, preserves existing output folders, and never packages a previous run's executable.

- `Logs/Pipeline/<run-id>/`: per-stage logs, editor log, NUnit XML when tests execute, and `pipeline-result.json` including status, stage duration, version, source commit when available, and errors.
- `Builds/Pipeline/<run-id>/Windows/`: the complete Windows player and Unity `build-report.json`, created only after tests pass.
- `Artifacts/<run-id>/`: the verified player ZIP, `.sha256` file, and pipeline result. A source-only task writes its source ZIP here instead.

Any failing stage stops subsequent stages and exits nonzero. Unity licensing initialization errors terminate the owned editor process rather than waiting for a native crash. Timeouts also terminate that process tree. A package requires a successful Unity report, matching version/configuration, executable, UnityPlayer DLL, player data, and the runtime assembly. It contains all player files, per-file hashes, and a package manifest; the archive is CRC/hash verified before its final name is published.

Run `Tools/Build-Windows.ps1` for compatibility with earlier commands; it now delegates to this pipeline and also runs tests before building. The Unity menu build remains available as an editor convenience, but does not replace the full pipeline's test gate.

After a development build, run the actual executable path printed by the `windows-build` stage with `--smoke-test --seed 1926 -screen-fullscreen 0`. The graphical smoke test is an explicit local playtest, not a headless CI test, and creates `QA` beside the executable. Its screenshot files are not generated or claimed by the build pipeline.

## GitHub Actions

`.github/workflows/build.yml` provides validation on pushes to `main`/`master`, version tags, pull requests, and manual runs. Validation uses a GitHub-hosted Windows runner. Windows builds use a licensed self-hosted Windows runner and the same `Build.ps1` command. Pull requests run source validation only; Windows builds run on pushes/tags/manual dispatch when enabled.

To enable Windows CI after putting this project in a GitHub repository:

1. Register a current GitHub Actions runner on a Windows build machine. Give it the additional label `unity-6000.4`; its normal labels are `self-hosted`, `Windows`, and `X64`.
2. Install the pinned Unity editor with Windows Mono support, PowerShell 7, and Python 3.11+. Confirm the runner account can start Unity and use its license. Use a dedicated runner checkout rather than an interactive development project.
3. Set repository variable `STILL_AWAKE_WINDOWS_CI` to `true`. Optionally set repository variable `STILL_AWAKE_UNITY_EDITOR` to a non-default editor path.
4. Run **Still Awake build** from the Actions tab. Choose Release or Development. The default is Release. The workflow preserves diagnostics on failure and uploads packages only on success.

The Windows job is intentionally disabled until the runner variable is set, so a repository without a Unity runner can still validate source. No repository/runner registration, GitHub push, scheduled job, or remote build was performed when these files were created. The current workspace is not connected to a Git remote.

The workflow uses the currently documented `actions/checkout@v7` and `actions/upload-artifact@v7`, read-only repository permission, and serialized Windows builds. Keep the self-hosted runner current enough to run the actions' Node runtime. Diagnostic retention is 14 days; packaged build retention is 30 days.

Official references: [Unity editor command line](https://docs.unity3d.com/6000.4/Documentation/Manual/EditorCommandLineArguments.html), [GitHub runner labels](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/use-in-a-workflow), [checkout](https://github.com/actions/checkout), [upload-artifact](https://github.com/actions/upload-artifact).

## Current environment

The direct Windows build requested on 6 November 2026 failed with Unity exit code `-536870911` after licensing initialization errors. The new full pipeline was then exercised: its four validation stages passed, it detected the licensing failure at the Unity test stage after about 77 seconds, terminated its editor, wrote a failed run report, and published no release artifact. The Compile task passed independently. An executable is not available until the Unity Test and Build stages succeed. Package locking is finalized by Unity's first successful import; top-level dependencies are pinned in `Packages/manifest.json`.
