# Windows playable build runbook

> Historical build attempts and troubleshooting. The root pipeline has now been restored: see [Local-Build.md](Local-Build.md) for current commands and paths. `npm run build` now defaults to Development. Older Shipping defaults, nested `Unreal/` paths, and ZIP behavior below describe earlier versions.

## Local npm command

From `D:\Still Awake`, run **`npm run build`**. No npm install is needed. The Node wrapper checks for PowerShell 7 at `STILL_AWAKE_PWSH` (when explicitly set), the installed Codex runtime path, the standard PowerShell installation, or PATH. It invokes the full `Build` task with `Shipping` and `-SkipZip`, preserving every test and cooking stage.

Success prints `Unreal/Builds/<run>/Windows/StillAwake.exe` and records its absolute path in `pipeline-result.json`. Keep the runtime folders next to the executable. Verification requires the launcher, game runtime executable, cooked PAK/IO Store files and authored content. A failed stage makes npm exit nonzero. This command creates no ZIP, uploads no files and does not bypass the engine startup restrictions described below.

The first stage checks access to Unreal's existing machine identity, or initialization access when it is missing. It opens registry handles only: it does not create keys, write values, reveal the ID, or modify permissions. If `unreal-environment` fails, use an ordinary Windows terminal with the required access. `-Task Validate` and compile-only tasks remain available for independent work.

## PowerShell version fix

`Build.ps1` requires PowerShell 7. Running it directly from Windows PowerShell 5.1 fails at `#requires -Version 7.0` before any build starts. PowerShell 7.6.9 was verified at the following location on November 9, 2026. The standard `C:\Program Files\PowerShell\7\pwsh.exe` location was absent.

Run this from an ordinary user PowerShell window, including Windows PowerShell 5.1:

```powershell
& 'C:\Users\Zayel\.cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe' -NoProfile -File 'D:\Still Awake\Build.ps1' -Task Build -Configuration Shipping -SkipZip
```

The command explicitly launches PowerShell 7, avoiding dependence on the shell already open or its PATH. This runtime location can change with Codex updates; verify it exists before reuse. No execution-policy or registry changes are part of this fix.

## Expected result

The full pipeline validates source, checks native rules, compiles the editor, generates assets, runs Unreal automation and physical traversal checks, then compiles/cooks/stages and verifies the Windows Shipping player. `-Task Editor` alone does not produce a playable distribution. ZIP creation is optional: omit `-SkipZip` only when explicitly requesting a distribution archive.

- Report and stage logs: `Unreal/Saved/Pipeline/<run>/`.
- Runnable player directory: `Unreal/Builds/<run>/Windows/`.
- Optional distributable archive: `Unreal/Artifacts/<run>/StillAwake-0.3.0-Unreal-Windows-Shipping.zip`, with SHA-256 sidecar.
- Extract the complete ZIP, then run `StillAwake.exe`; retain the accompanying folders/files.

Check the final `pipeline-result.json` status and artifacts (the verified Windows folder for npm builds; a ZIP when requested). A source ZIP or an uncooked executable is not a playable release. Verify launch separately before claiming it has been playtested.

## Session-specific startup limitation and current evidence

The assistant-managed build `20261109-183855-3b69655c` passed source validation, 38 native checks and editor compilation, but Unreal exited without generating assets or a startup log. A read of `HKCU:\Software\Epic Games\Unreal Engine\Identifiers` returned access denied, consistent with the earlier MachineId initialization failure documented in `Validation.md`. The PowerShell-version fix does not repair or authorize bypassing registry permissions.

A newer user-started run, `20261109-184352-4c4bcc22`, reached content preparation and Unreal automation tests on November 9, 2026. At the time this note was written, `UnrealEditor-Cmd` was active, no final pipeline report existed, and packaging success was not established. Read the latest logs for current status rather than treating this snapshot as completion. This demonstrates progress in the user's ordinary session, not a verified change to assistant-session permissions.

Do not start concurrent pipelines, alter registry ACLs, patch the engine to evade access restrictions, or call compilation alone a successful playable build. If blocked, report the failing stage and exact error, preserve the logs, and distinguish checks passed from checks never run.

## November 9 build outcome and recovered playable package

Run `20261109-184352-4c4bcc22` passed native checks, content preparation, Unreal automation, and 46 physical-traversal checks. BuildCookRun compiled, cooked, staged, and archived successfully, then returned failure because the nested `-ubtargs` value split the spaced UBA path into an extra AutomationTool command (`Awake\Unreal\Saved\UBA`). The pipeline's argument quoting still needs correction before reliable unattended full-pipeline completion; do not mark its failed report as passed.

The archived Shipping executable was subsequently launched with the headless smoke harness and passed all 46 checks. `Tools/Package.py player` recovered and integrity-checked the single player ZIP under `Artifacts/20261109-184352-4c4bcc22/`. See `recovery-validation.json` there for the exact path, size, checksum, and verification evidence. Visual/audio playtesting was not performed. Preserve the original failed pipeline report alongside this separate recovery evidence.

## Quoting correction and bounded verification

The pipeline now invokes the installed AutomationTool DLL using Unreal 5.8.2's bundled .NET 10 host, avoiding the batch-file quote re-parsing that split paths containing spaces. Its .NET environment is set explicitly. AutomationTool logs now use a dedicated subdirectory so its log cleanup cannot remove earlier test evidence or contend with the stage's output log.

`Tools/Test-BuildArguments.ps1` extracts the production `Invoke-Step` function and runs AutomationTool's BuildCookRun help with the real nested UBA/UserDir arguments. This bounded test checks exit success and preserved quoted paths; it does not compile, cook, or package. It passed after the fix. A new full pipeline run with this corrected invocation has NOT been performed. The recovered playable ZIP was generated before this build-script correction and its separate runtime/ZIP validation remains valid.


## November 10 UTC: Toad update, v0.3

The active source now implements the three-room house, ten anomalies and countdown from 10. Earlier recovered package/test evidence above belongs to v0.2 only. See [Toad-Update-v0.3.md](Toad-Update-v0.3.md) for the requirement mapping and current checks.

The new full Shipping attempt `20261109-201715-0bfbcc69` passed native checks and editor compilation, then stopped during engine startup/content preparation. The log was empty and no fresh v0.3 content-ready marker was produced. Existing map/material files are now explicitly insufficient to pass that stage. The MachineId registry read still returned access denied in the assistant session; no permissions or engine changes were made. The corrected AutomationTool invocation was not reached.

Run the same full Shipping command near the top of this runbook from the ordinary user session. Require the final passed report, the new 44-check physical smoke result, verified cooked v0.3 Windows folder (or explicitly requested ZIP), and separate launch verification. A source ZIP or an uncooked Shipping compilation is not that outcome.

## November 10: npm entry point verification

`npm run build` was exercised locally in run `20261110-114838-3ef3e124`. It found PowerShell 7, selected `Build` / `Shipping` / `skipZip: true`, passed source validation, all 38 native rule checks and editor compilation, then correctly returned exit code 1 when Unreal failed to confirm content preparation. No ZIP or new playable v0.3 executable was produced by that run.

The local-player verifier passed against the existing v0.2 player folder without changing it, accepted a complete isolated fixture, rejected six fixtures missing required runtime components, and wrote no archives. A missing PowerShell override also returned exit code 1 with an actionable error. These entry-point checks do not establish a completed v0.3 build or launch.

## November 10: confirmed startup crash and pipeline repair

A direct launch again returned zero with no stdout, stderr or engine log. Read-only debugging then captured the underlying fault: `FWindowsPlatformMisc::SetStoredValue` could not save `MachineId` (Windows error 5, access denied), followed by access violation `0xC0000005` in `UnrealEditor-Core.dll`. The installed engine source confirms this identity is loaded from `HKCU:\Software\Epic Games\Unreal Engine\Identifiers` before project content generation. Evidence is under `Saved/Pipeline/startup-diagnostic-20261110-172639/`.

The pipeline now checks this prerequisite before compiling and verifies content/test outputs before printing or recording a passed stage. A zero process exit with missing outputs is a failure. Native process exit codes are included in the stage report.

Validation run `20261110-173300-bfd671fa` passed all 11 new pipeline checks, source validation and 38 native rule checks. The AutomationTool argument regression also passed. The subsequent `npm run build` run `20261110-173351-a1995d71` correctly stopped at `unreal-environment` with the exact access-denied error, without compiling or starting Unreal again.

This repairs prerequisite detection and misleading stage reporting. It does **not** remove the Windows session restriction or establish a successful playable build. No registry permissions, security settings or engine files were changed, and no new executable, archive or upload was produced. The remaining full build and in-game verification require a Windows session in which Unreal can access its normal user registry key.
