# Migration validation — 2026-11-07

The native Unreal C++ implementation is complete enough to compile. Runtime and packaging validation remain blocked by the managed authoring environment. Do not interpret a successful C++ compile as a finished player build.

| Check | Result |
|---|---|
| Authored content and staging configuration | Passed: 32 unique anomalies, eight rooms, six story fragments |
| Standalone native gameplay acceptance checks | Passed: 38 checks, zero failures |
| Unreal 5.8.2 editor module | Compiled and linked successfully |
| Unreal 5.8.2 Windows Development game target | Compiled and linked successfully; executable requires cooked assets before distribution |
| PowerShell pipeline syntax | Passed |
| Editor asset generation | Blocked before the editor initializes |
| Unreal automation / physical traversal / visual and audio review | Not run because the editor cannot start |
| Cooking / staged Windows player | Not completed |

The final editor module was compiled and linked by UnrealBuildTool. After an accelerator stall during the final game rebuild, the same generated compiler/linker response files were executed with the installed MSVC toolchain; the final Windows Development executable linked successfully. It remains an uncooked executable, not a distributable game.

## Confirmed environment blocker

The installed Unreal editor exits before writing its normal startup log. Read-only startup debugging resolved an access violation inside `UnrealEditor-Core.dll`, while Unreal tried to report this error:

```
FWindowsPlatformMisc::SetStoredValue: ERROR: Could not store value for 'MachineId'. Error Code 5: Access is denied.
```

Unreal's startup registry initialization fails under the managed session's permissions; its early error handler then crashes. No registry permissions or engine binaries were changed. Running the build from an ordinary user PowerShell session is the next step. Administrator privileges should not be necessary for Unreal's own current-user settings.

```powershell
cd 'D:\Still Awake'
.\Build.ps1
```

This will compile the editor module, create the map/material, run Unreal automation and the physical route integration test, then compile/cook/package the Windows player. The first run can take several minutes for shader compilation. Inspect the per-stage logs and `pipeline-result.json` under `Unreal/Saved/Pipeline/` if it fails. The script deliberately rejects a zero-exit editor launch that produces no assets or test reports.

## Test coverage included

The 38 engine-independent checks exercise clean completion, correct retreats from all eight rooms, false alarms, missed/late/premature reversals, invalid route ordering, duplicate resolution, pause, tutorial and streak progression, deterministic seeded sampling, tier eligibility, recent-anomaly/room exclusion, weighted selection and distribution.

The opt-in native integration harness additionally checks all 32 anomaly effects and baseline restoration, both tutorials, pause, collision-based traversal of a clean circuit and eight return routes, the eight-success ending and save reload. It is included in `-Task Test` and the default build, but has not been executed successfully in this environment. Its accelerated walking speed is active only in the explicit smoke-test process.
