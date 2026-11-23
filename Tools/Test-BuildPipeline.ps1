#requires -Version 7.0
param([string]$ResultsDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
. (Join-Path $PSScriptRoot 'Test-UnrealEnvironment.ps1')
$checks=0
function Check([bool]$Valid,[string]$Name){
    if(!$Valid){throw "FAIL: $Name"}
    $script:checks++
}
function Expect-Failure([scriptblock]$Action,[string]$Message){
    $failed=$false
    try { & $Action | Out-Null }
    catch {
        if($_.Exception.Message -notlike "*$Message*"){throw}
        $failed=$true
    }
    Check $failed "Expected failure: $Message"
}
function New-KeyFixture($Value){
    $key=[pscustomobject]@{Value=$Value;Disposed=$false}
    $key | Add-Member -MemberType ScriptMethod -Name GetValue -Value {param($Name) $this.Value}
    $key | Add-Member -MemberType ScriptMethod -Name Dispose -Value {$this.Disposed=$true}
    return $key
}

# All registry cases use fixtures: these tests never create or alter registry keys.
$existing=Test-UnrealRegistryAccess -OpenKey {
    param($Path,$Writable)
    if($Writable){throw 'A valid existing ID must not require write permission.'}
    New-KeyFixture '11111111222233334444555555555555'
}
Check ($existing -like '*existing machine identity*') 'Valid read-only identity works'
$firstLaunch=Test-UnrealRegistryAccess -OpenKey {
    param($Path,$Writable)
    if($Writable -and $Path -eq 'Software'){New-KeyFixture $null}
}
Check ($firstLaunch -like '*initialize*') 'First launch accepts an accessible parent without creating keys'
$invalid=Test-UnrealRegistryAccess -OpenKey {
    param($Path,$Writable)
    New-KeyFixture 'invalid-existing-id'
}
Check ($invalid -like '*initialize*') 'Invalid ID can be replaced by Unreal when access is available'
Expect-Failure {
    Test-UnrealRegistryAccess -OpenKey {param($Path,$Writable) throw [UnauthorizedAccessException]::new('Access denied')}
} 'ordinary Windows terminal'
Expect-Failure {
    Test-UnrealRegistryAccess -OpenKey {
        param($Path,$Writable)
        if($Writable){throw [UnauthorizedAccessException]::new('Write access denied')}
        New-KeyFixture $null
    }
} 'Write access denied'

# Exercise the production stage runner, including zero-exit / missing-output failures.
$tokens=$null;$parseErrors=$null
$ast=[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Build.ps1'),[ref]$tokens,[ref]$parseErrors)
Check (!$parseErrors) 'Build script parses'
foreach($name in @('Quote-PS','ConvertTo-UnrealCommandLine','Invoke-Step','Assert-AutomationReport','Assert-SmokeReport')){
    $node=$ast.Find({param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name},$true)
    if(!$node){throw "Missing production function: $name"}
    . ([scriptblock]::Create($node.Extent.Text))
}
$projectRoot=$root
# UAT trims terminal quotes from pass-through arguments before appending its flags.
foreach($name in @('ubtArgs','cookerArgs')){
    $node=$ast.Find({param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] -and
        $n.Left -is [System.Management.Automation.Language.VariableExpressionAst] -and
        $n.Left.VariablePath.UserPath -eq $name},$true)
    if(!$node){throw "Missing production arguments: $name"}
    . ([scriptblock]::Create($node.Extent.Text))
}
$forwardedUbt=$ubtArgs.TrimStart('"').TrimEnd('"')
$forwardedCooker=$cookerArgs.TrimStart('"',' ').TrimEnd('"',' ')
Check ($forwardedUbt.Contains('-UBARootDir="'+$projectRoot+'/Saved/UBA"')) 'UAT preserves the quoted UBA path'
Check ($forwardedCooker.Contains('-UserDir="'+$projectRoot+'/Saved/BuildUser"')) 'UAT preserves the quoted cooker user path'
$logRoot=if($ResultsDirectory){[IO.Path]::GetFullPath($ResultsDirectory)}else{Join-Path $root ('Saved\BuildTests\'+[Guid]::NewGuid().ToString('N'))}
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
$summary=[ordered]@{stages=@()}
$TimeoutSeconds=30
$uatHost='not-used-by-this-test'
$powershell=Join-Path $PSHOME 'pwsh.exe'
$script:verified=$false
Invoke-Step 'verified-success' $powershell @('-NoProfile','-Command','Write-Output ''completed''; exit 0') -Verify {$script:verified=$true}
Check ($script:verified -and $summary.stages[-1].status -eq 'Passed') 'Success requires post-process verification'
Expect-Failure {
    Invoke-Step 'zero-exit-missing-output' $powershell @('-NoProfile','-Command','exit 0') -Verify {throw 'Expected output missing'}
} 'Expected output missing'
Check ($summary.stages[-1].status -eq 'Failed' -and $summary.stages[-1].exitCode -eq 0) 'Zero-exit process with missing output is a failed stage'
Expect-Failure {
    Invoke-Step 'process-failure' $powershell @('-NoProfile','-Command','exit 17')
} 'exited with code 17'
Check ($summary.stages[-1].status -eq 'Failed' -and $summary.stages[-1].exitCode -eq 17) 'Nonzero exit is preserved'

# Inspect the raw child command line: Unreal parses this directly instead of argv.
$probe=Join-Path $logRoot 'Raw command line.ps1'
@'
Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public static class RawCommandLine { [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr GetCommandLineW(); }'
Write-Output ([Runtime.InteropServices.Marshal]::PtrToStringUni([RawCommandLine]::GetCommandLineW()))
'@ | Set-Content -LiteralPath $probe
Invoke-Step 'unreal-quoting' $powershell @('-NoProfile','-File',$probe,
    '-ExecCmds=Automation RunTests StillAwake',"-ReportExportPath=$logRoot/Path With Spaces") -UnrealArguments
$raw=Get-Content (Join-Path $logRoot 'unreal-quoting.log') -Raw
Check ($raw.Contains('-ExecCmds="Automation RunTests StillAwake"')) 'Unreal receives quoted command value'
Check ($raw.Contains('-ReportExportPath="'+$logRoot+'/Path With Spaces"')) 'Unreal receives complete spaced path'
Expect-Failure { ConvertTo-UnrealCommandLine @('-Key=embedded"quote') } 'embedded quotes'

# A hung child must fail within the configured bound and remain a failed stage.
$TimeoutSeconds=1
Expect-Failure {
    Invoke-Step 'timeout' $powershell @('-NoProfile','-Command','Start-Sleep -Seconds 30')
} 'timed out'
Check ($summary.stages[-1].status -eq 'Failed') 'Timeout is recorded as failure'
$TimeoutSeconds=30

# Exit zero alone cannot substitute for completed engine test reports.
$automationReport=Join-Path $logRoot 'automation-fixture.json'
Expect-Failure { Assert-AutomationReport $automationReport } 'report is missing'
@{failed=0;tests=@()} | ConvertTo-Json | Set-Content $automationReport
Expect-Failure { Assert-AutomationReport $automationReport } 'required test'
$tests=@(
    @{fullTestPath='StillAwake.Rules.Acceptance';state='Success'},
    @{fullTestPath='StillAwake.Content.AuthoredLibrary';state='Success'},
    @{fullTestPath='StillAwake.Build.Identity';state='Success'}
)
@{failed=0;tests=$tests} | ConvertTo-Json -Depth 5 | Set-Content $automationReport
Assert-AutomationReport $automationReport
Check $true 'Complete automation report accepted'
@{failed=0;tests=@($tests | Where-Object fullTestPath -NE 'StillAwake.Build.Identity')} | ConvertTo-Json -Depth 5 | Set-Content $automationReport
Expect-Failure { Assert-AutomationReport $automationReport } 'StillAwake.Build.Identity'
$tests[1].state='Fail'
@{failed=1;tests=$tests} | ConvertTo-Json -Depth 5 | Set-Content $automationReport
Expect-Failure { Assert-AutomationReport $automationReport } 'required test'
$smokeReport=Join-Path $logRoot 'smoke-fixture.json'
Expect-Failure { Assert-SmokeReport $smokeReport } 'report is missing'
@{passed=$true;checks=43} | ConvertTo-Json | Set-Content $smokeReport
Expect-Failure { Assert-SmokeReport $smokeReport } '44 required checks'
@{passed=$false;checks=44} | ConvertTo-Json | Set-Content $smokeReport
Expect-Failure { Assert-SmokeReport $smokeReport } '44 required checks'
@{passed=$true;checks=44} | ConvertTo-Json | Set-Content $smokeReport
Assert-SmokeReport $smokeReport
Check $true 'Complete smoke report accepted'
$identity=@{version='0.3.0';buildNumber='12';buildVersion='0.3.0+build.12';runId='fixture';configuration='Shipping'}
Expect-Failure { Assert-SmokeReport $smokeReport $identity } 'build identity differs'
$stamped=@{passed=$true;checks=44}+ $identity
$stamped | ConvertTo-Json | Set-Content $smokeReport
Assert-SmokeReport $smokeReport $identity
Check $true 'Smoke report confirms the expected packaged build'
$stamped.buildNumber='11'
$stamped | ConvertTo-Json | Set-Content $smokeReport
Expect-Failure { Assert-SmokeReport $smokeReport $identity } 'build identity differs: buildNumber'
@{passed=$true;checks=$checks;stages=$summary.stages} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $logRoot 'regression-result.json')
Write-Host "PASS: $checks build pipeline checks. No Unreal process or registry mutation. Report: $logRoot"
