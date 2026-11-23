#requires -Version 7.0
[CmdletBinding()]
param(
    [ValidateSet('Build', 'Validate', 'Compile', 'Editor', 'Test')]
    [string]$Task = 'Build',
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration = 'Development',
    [string]$EngineRoot = $env:STILL_AWAKE_UNREAL_ENGINE,
    [string]$Python = 'python',
    [ValidateRange(1, 14400)][int]$TimeoutSeconds = 3600,
    # Compatibility with old local commands: this pipeline never creates ZIPs.
    [switch]$SkipZip
)
$ErrorActionPreference = 'Stop'

function Quote-PS([string]$Value) {
    return "'" + $Value.Replace("'", "''") + "'"
}

function ConvertTo-UnrealCommandLine([string[]]$Values) {
    # Unreal reads the raw Windows command line, not the CRT/.NET argv array.
    # FParse::Value requires -Key="value with spaces", not "-Key=value with spaces".
    $tokens = foreach ($value in $Values) {
        if ($value -match '["\r\n]') { throw 'Unreal arguments cannot contain embedded quotes or newlines.' }
        if ($value -match '^(-[^=\s]+)=(.*)$') { $Matches[1] + '="' + $Matches[2] + '"' }
        elseif ($value -match '\s') { '"' + $value + '"' }
        else { $value }
    }
    return $tokens -join ' '
}

function Invoke-Step {
    param(
        [string]$Name, [string]$Executable, [string[]]$Arguments,
        [scriptblock]$Verify, [string]$WorkingDirectory = $projectRoot,
        [switch]$UnrealArguments
    )
    $stage = [ordered]@{ name = $Name; status = 'Running'; exitCode = $null; durationSeconds = 0;
        log = "$Name.log"; stderr = "$Name.stderr.log"; error = $null }
    $summary.stages += $stage
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $process = [Diagnostics.Process]::new()
    $stdout = $null; $stderr = $null; $started = $false
    try {
        New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
        $process.StartInfo = [Diagnostics.ProcessStartInfo]::new()
        $info = $process.StartInfo
        $info.FileName = $Executable
        $info.WorkingDirectory = $WorkingDirectory
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $info.RedirectStandardOutput = $true
        $info.RedirectStandardError = $true
        if ($UnrealArguments) { $info.Arguments = ConvertTo-UnrealCommandLine $Arguments }
        else { foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) } }
        # Match Unreal's bundled .NET setup without batch-file argument reparsing.
        if ([IO.Path]::GetFileName($Executable) -eq 'dotnet.exe') {
            $dotnetRoot = Split-Path $Executable
            $info.Environment['DOTNET_ROOT'] = $dotnetRoot
            $info.Environment['DOTNET_MULTILEVEL_LOOKUP'] = '0'
            $info.Environment['DOTNET_ROLL_FORWARD'] = 'LatestMajor'
            $info.Environment['PATH'] = "$dotnetRoot;$env:PATH"
            $info.Environment['uebp_LogFolder'] = Join-Path $logRoot "UAT-$Name"
            $info.Environment['uebp_FinalLogFolder'] = Join-Path $logRoot "UAT-$Name"
            $info.Environment['uebp_EngineSavedFolder'] = Join-Path $projectRoot 'Saved/AutomationTool'
        }
        $stdout = [IO.File]::Open((Join-Path $logRoot $stage.log), 'Create', 'Write', 'ReadWrite')
        $stderr = [IO.File]::Open((Join-Path $logRoot $stage.stderr), 'Create', 'Write', 'ReadWrite')
        Write-Host "[$Name] Starting. Log: $(Join-Path $logRoot $stage.log)"
        $started = $process.Start()
        if (!$started) { throw "Could not start $Executable" }
        $copyOut = $process.StandardOutput.BaseStream.CopyToAsync($stdout)
        $copyErr = $process.StandardError.BaseStream.CopyToAsync($stderr)
        $nextUpdate = 30
        while (!$process.WaitForExit(1000)) {
            if ($timer.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
                $process.Kill($true)
                $process.WaitForExit()
                throw "Stage '$Name' timed out after $TimeoutSeconds seconds."
            }
            if ($timer.Elapsed.TotalSeconds -ge $nextUpdate) {
                Write-Host "[$Name] Running ($([int]$timer.Elapsed.TotalSeconds)s); output is being written to the log."
                $nextUpdate += 30
            }
        }
        $stage.exitCode = $process.ExitCode
        $copies = [Threading.Tasks.Task]::WhenAll([Threading.Tasks.Task[]]@($copyOut, $copyErr))
        if (!$copies.Wait([TimeSpan]::FromSeconds(10))) { throw "Stage '$Name' output did not close." }
        $stdout.Dispose(); $stdout = $null
        $stderr.Dispose(); $stderr = $null
        if ($stage.exitCode -ne 0) { throw "Stage '$Name' exited with code $($stage.exitCode)." }
        if ($Verify) { & $Verify }
        $stage.status = 'Passed'
        Write-Host "[$Name] Passed."
    } catch {
        $stage.status = 'Failed'
        $stage.error = $_.Exception.Message
        foreach ($file in @($stage.log, $stage.stderr)) {
            $path = Join-Path $logRoot $file
            if (Test-Path -LiteralPath $path) { Get-Content -LiteralPath $path -Tail 15 | Write-Host }
        }
        throw
    } finally {
        if ($started -and !$process.HasExited) { $process.Kill($true); $process.WaitForExit() }
        if ($started -and $null -eq $stage.exitCode) { $stage.exitCode = $process.ExitCode }
        if ($stdout) { $stdout.Dispose() }
        if ($stderr) { $stderr.Dispose() }
        $process.Dispose()
        $stage.durationSeconds = [Math]::Round($timer.Elapsed.TotalSeconds, 2)
    }
}

function Assert-AutomationReport([string]$Path) {
    if (!(Test-Path -LiteralPath $Path)) { throw 'Unreal automation report is missing.' }
    $report = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    $expected = @('StillAwake.Rules.Acceptance', 'StillAwake.Content.AuthoredLibrary', 'StillAwake.Build.Identity')
    foreach ($name in $expected) {
        $matches = @($report.tests | Where-Object fullTestPath -EQ $name)
        if ($matches.Count -ne 1 -or $matches[0].state -ne 'Success') {
            throw "Unreal automation report did not pass required test: $name"
        }
    }
    if ($report.failed -gt 0 -or @($report.tests | Where-Object state -NE 'Success').Count -gt 0) {
        throw 'Unreal automation report contains failed or unfinished tests.'
    }
}

function Assert-SmokeReport([string]$Path, $BuildInfo = $null) {
    if (!(Test-Path -LiteralPath $Path)) { throw 'Smoke report is missing.' }
    $report = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    if ($report.passed -isnot [bool] -or !$report.passed -or $report.checks -lt 44) {
        throw 'Smoke report did not pass all 44 required checks.'
    }
    if (!$BuildInfo) { return }
    foreach ($name in $BuildInfo.Keys) {
        if ($report.$name -cne $BuildInfo[$name]) { throw "Smoke report build identity differs: $name" }
    }
}

$projectRoot = $PSScriptRoot
$project = Join-Path $projectRoot 'StillAwake.uproject'
$runId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)
$logRoot = Join-Path $projectRoot "Saved/Pipeline/$runId"
$buildRoot = Join-Path $projectRoot "Builds/$runId"
$playerRoot = Join-Path $buildRoot 'Windows'
$summary = [ordered]@{
    runId = $runId; task = $Task; configuration = $Configuration; status = 'Running'
    startedUtc = [DateTime]::UtcNow.ToString('o'); stages = @(); artifacts = @(); error = $null
}
$lock = $null
$resultCode = 1
try {
    if (!$IsWindows) { throw 'Windows is required for this build pipeline.' }
    New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
    # OS-held lock releases automatically even after a process crash.
    try { $lock = [IO.File]::Open((Join-Path $projectRoot 'Saved/build.lock'), 'OpenOrCreate', 'ReadWrite', 'None') }
    catch { throw "Another pipeline is running, or the project build lock is inaccessible: $($_.Exception.Message)" }
    if ($env:GITHUB_OUTPUT) { "log-directory=$logRoot" | Add-Content -LiteralPath $env:GITHUB_OUTPUT }
    $powershell = Join-Path $PSHOME 'pwsh.exe'
    $pythonExe = (Get-Command $Python -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
    . (Join-Path $projectRoot 'Tools/Version.ps1')
    $summary.version = Get-ProjectVersion -Path (Join-Path $projectRoot 'Config/DefaultGame.ini')
    if ($Task -eq 'Build') {
        $summary.buildNumber = Reserve-BuildNumber -Path (Join-Path $projectRoot 'Saved/BuildNumber.txt')
        $summary.buildVersion = Get-BuildVersion -Version $summary.version -BuildNumber $summary.buildNumber
        Write-Host "Build version: $($summary.buildVersion)"
    }
    Invoke-Step 'source-validation' $pythonExe @((Join-Path $projectRoot 'Tools/Verify-Project.py'))
    $versionTests = Join-Path $projectRoot ".tmp/Version/$runId"
    Invoke-Step 'version-regression' $powershell @('-NoProfile', '-File', (Join-Path $projectRoot 'Tools/Test-Version.ps1'),
        '-ResultsDirectory', $versionTests) -Verify {
            $report = Get-Content (Join-Path $versionTests 'version-result.json') -Raw | ConvertFrom-Json
            if ($report.passed -isnot [bool] -or !$report.passed -or $report.checks -lt 82) {
                throw 'Version regression report is incomplete.'
            }
        }
    Invoke-Step 'package-regression' $pythonExe @((Join-Path $projectRoot 'Tests/PackageTest.py'))
    Invoke-Step 'native-rules' $powershell @('-NoProfile', '-File', (Join-Path $projectRoot 'Tools/Test-Rules.ps1'))
    Invoke-Step 'pipeline-regression' $powershell @('-NoProfile', '-File', (Join-Path $projectRoot 'Tools/Test-BuildPipeline.ps1'),
        '-ResultsDirectory', (Join-Path $logRoot 'Regression'))

    if ($Task -ne 'Validate') {
        if (!$EngineRoot) { $EngineRoot = Join-Path $env:ProgramFiles 'Epic Games/UE_5.8' }
        $EngineRoot = (Resolve-Path -LiteralPath $EngineRoot).Path
        $engine = Get-Content (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
        $summary.engineVersion = "$($engine.MajorVersion).$($engine.MinorVersion).$($engine.PatchVersion)"
        if ($summary.engineVersion -ne '5.8.2') { throw "Unreal 5.8.2 is required; found $($summary.engineVersion)." }
        $uatHost = Join-Path $EngineRoot 'Engine/Binaries/ThirdParty/DotNet/10.0/win-x64/dotnet.exe'
        $ubt = Join-Path $EngineRoot 'Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll'
        $uat = Join-Path $EngineRoot 'Engine/Binaries/DotNET/AutomationTool/AutomationTool.dll'
        $editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
        foreach ($file in @($uatHost, $ubt, $uat, $editor)) {
            if (!(Test-Path -LiteralPath $file)) { throw "Required Unreal tool is missing: $file" }
        }
        # UBT can compile independently of editor startup/registry access.
        if ($Task -in @('Build', 'Test')) {
            Invoke-Step 'unreal-environment' $powershell @('-NoProfile', '-File', (Join-Path $projectRoot 'Tools/Test-UnrealEnvironment.ps1'))
        }
        $sessionId = (Get-Process -Id $PID).SessionId
        $editorProcesses = @(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue |
            Where-Object SessionId -EQ $sessionId)
        if ($editorProcesses.Count -gt 0) { throw 'Close Unreal Editor in this Windows session before running the pipeline.' }
        Invoke-Step 'editor-compile' $uatHost @($ubt, 'StillAwakeEditor', 'Win64', 'Development', "-Project=$project",
            '-WaitMutex', '-NoHotReloadFromIDE', '-UBADisableRemote', "-UBARootDir=$projectRoot/Saved/UBA",
            "-Log=$logRoot/UBT-Editor.log") -WorkingDirectory (Join-Path $EngineRoot 'Engine/Source') -Verify {
            $dll = Join-Path $projectRoot 'Binaries/Win64/UnrealEditor-StillAwake.dll'
            if (!(Test-Path -LiteralPath $dll) -or (Get-Item -LiteralPath $dll).Length -eq 0) {
                throw 'Editor compilation did not produce its game module.'
            }
            if ((Get-Content "$logRoot/UBT-Editor.log" -Raw) -notmatch 'Result: Succeeded') {
                throw 'UnrealBuildTool did not report a successful editor build.'
            }
        }
    }
    if ($Task -in @('Build', 'Test')) {
        $editorOptions = @('-unattended', '-nop4', '-nosplash', '-nullrhi', '-nosound', '-stdout', '-FullStdOutLogOutput',
            '-DDC=StillAwake', "-UserDir=$projectRoot/Saved/BuildUser")
        Invoke-Step 'prepare-content' $editor (@($project, '-run=pythonscript', "-script=$projectRoot/Tools/Prepare-Content.py",
            "-AbsLog=$logRoot/PrepareContent.log") + $editorOptions) -UnrealArguments -Verify {
            if (!(Test-Path "$logRoot/PrepareContent.log") -or
                (Get-Content "$logRoot/PrepareContent.log" -Raw) -notmatch 'STILL AWAKE CONTENT READY v0.3:') {
                throw 'Unreal did not confirm fresh content preparation. See PrepareContent.log.'
            }
            foreach ($asset in @('Content/StillAwake/Maps/House.umap', 'Content/StillAwake/M_House.uasset')) {
                if (!(Test-Path (Join-Path $projectRoot $asset))) { throw "Prepared asset missing: $asset" }
            }
        }
        $automation = Join-Path $logRoot 'Automation'
        Invoke-Step 'unreal-tests' $editor (@($project, '-ExecCmds=Automation RunTests StillAwake',
            '-TestExit=Automation Test Queue Empty', "-ReportExportPath=$automation", "-AbsLog=$logRoot/Automation.log") +
            $editorOptions) -UnrealArguments -Verify { Assert-AutomationReport (Join-Path $automation 'index.json') }
        $editorSmoke = Join-Path $logRoot 'editor-smoke.json'
        Invoke-Step 'editor-smoke' $editor (@($project, '/Game/StillAwake/Maps/House', '-game', '-SmokeTest', '-Seed=1926',
            "-SmokeReport=$editorSmoke", "-AbsLog=$logRoot/EditorSmoke.log") + $editorOptions) -UnrealArguments -Verify {
            Assert-SmokeReport $editorSmoke
        }
    }
    if ($Task -eq 'Build') {
        $ubtArgs = "-UBARootDir=`"$projectRoot/Saved/UBA`" -UBADisableRemote"
        $cookerArgs = "-UserDir=`"$projectRoot/Saved/BuildUser`" -DDC=StillAwake"
        Invoke-Step 'package-windows' $uatHost @($uat, 'BuildCookRun', "-project=$project", '-noP4', '-unattended',
            '-utf8output', '-platform=Win64', "-clientconfig=$Configuration", '-build', '-cook', '-stage', '-pak', '-iostore',
            '-package', '-archive', "-archivedirectory=$buildRoot", "-stagingdirectory=$projectRoot/Saved/StagedBuilds/$runId",
            '-prereqs', "-ubtargs=$ubtArgs", "-AdditionalCookerOptions=$cookerArgs") -Verify {
            if ((Get-Content "$logRoot/package-windows.log" -Raw) -notmatch 'AutomationTool exiting with ExitCode=0 \(Success\)') {
                throw 'AutomationTool did not confirm successful packaging.'
            }
        }
        $buildInfo = [ordered]@{ version = $summary.version; buildNumber = [string]$summary.buildNumber;
            buildVersion = $summary.buildVersion; runId = $runId; configuration = $Configuration }
        $buildInfo | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $playerRoot 'StillAwake/build-info.json') -Encoding utf8
        Invoke-Step 'verify-player' $pythonExe @((Join-Path $projectRoot 'Tools/Package.py'), 'verify-player', $playerRoot)
        $smoke = Join-Path $logRoot 'packaged-smoke.json'
        # Use the runtime executable so the pipeline owns the process and its exit code.
        $runtimeName = if ($Configuration -eq 'Shipping') { 'StillAwake-Win64-Shipping.exe' } else { 'StillAwake.exe' }
        $runtime = Join-Path $playerRoot "StillAwake/Binaries/Win64/$runtimeName"
        Invoke-Step 'packaged-smoke' $runtime @('-SmokeTest', '-Seed=1926', '-nullrhi', '-nosound', '-unattended',
            "-SmokeReport=$smoke", "-UserDir=$logRoot/PackagedTestUser", "-AbsLog=$logRoot/PackagedSmoke.log") -UnrealArguments -Verify {
            Assert-SmokeReport $smoke $buildInfo
        }
        $summary.executable = Join-Path $playerRoot 'StillAwake.exe'
        $summary.artifacts = @($playerRoot)
        $latest = Join-Path $projectRoot "Builds/latest-$($Configuration.ToLowerInvariant()).json"
        $pointer = @{ executable = $summary.executable; configuration = $Configuration; version = $summary.version;
            buildNumber = $summary.buildNumber; buildVersion = $summary.buildVersion;
            report = "$logRoot/pipeline-result.json"; runId = $runId }
        if ($env:GITHUB_OUTPUT) { "artifact-directory=$playerRoot" | Add-Content -LiteralPath $env:GITHUB_OUTPUT }
        $pointer | ConvertTo-Json | Set-Content -LiteralPath "$latest.tmp" -Encoding utf8
        [IO.File]::Move("$latest.tmp", $latest, $true)
    }
    $summary.status = 'Passed'
    $resultCode = 0
} catch {
    $summary.status = 'Failed'
    $summary.error = $_.Exception.Message
    [Console]::Error.WriteLine("Build failed: $($summary.error)")
} finally {
    $summary.finishedUtc = [DateTime]::UtcNow.ToString('o')
    if (Test-Path -LiteralPath $logRoot) {
        $summary | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$logRoot/pipeline-result.json" -Encoding utf8
        Write-Host "Report: $logRoot/pipeline-result.json"
    }
    if ($lock) { $lock.Dispose() }
}
if ($resultCode -eq 0) {
    Write-Host "PASS: $Task / $Configuration"
    if ($Task -eq 'Build') { Write-Host "Playable executable: $($summary.executable)" }
}
exit $resultCode
