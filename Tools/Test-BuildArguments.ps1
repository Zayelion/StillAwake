#requires -Version 7.0
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$tokens=$null;$errors=$null
$ast=[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Build.ps1'),[ref]$tokens,[ref]$errors)
if($errors){throw ($errors | Out-String)}
foreach($name in @('Quote-PS','Invoke-Step')){
    $node=$ast.Find({param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -eq $name},$true)
    if(!$node){throw "Missing function $name"}
    . ([scriptblock]::Create($node.Extent.Text))
}
$projectRoot=$root
$logRoot=Join-Path $root ('Saved\Pipeline\ArgumentRegression-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Force $logRoot | Out-Null
$summary=[ordered]@{stages=@()}
$TimeoutSeconds=120
$engineRoot=if($env:STILL_AWAKE_UNREAL_ENGINE){$env:STILL_AWAKE_UNREAL_ENGINE}else{'C:\Program Files\Epic Games\UE_5.8'}
$uat=Join-Path $engineRoot 'Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
$uatHost=Join-Path $engineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
$ubtArgs="-UBADisableRemote -UBARootDir=`"$root\Saved\UBA`""
$cookArgs="-DDC=StillAwake -UserDir=`"$root\Saved\BuildUser`""
# Help parses these exact nested values without compiling, cooking, or packaging.
Invoke-Step 'argument-regression' $uatHost @($uat,'BuildCookRun','-Help',"-ubtargs=$ubtArgs","-AdditionalCookerOptions=$cookArgs")
$log=Get-Content (Join-Path $logRoot 'argument-regression.log') -Raw
if($log -notmatch 'AutomationTool exiting with ExitCode=0 \(Success\)' -or $log -match 'Failed to find command'){throw 'AutomationTool argument/help regression failed.'}
foreach($value in @($ubtArgs,$cookArgs)){
    if(!$log.Contains($value.Replace('"','\"'))){throw "Nested argument was not preserved: $value"}
}
Write-Host "PASS: AutomationTool Help accepted nested paths with spaces; no full build run. Log: $logRoot"

