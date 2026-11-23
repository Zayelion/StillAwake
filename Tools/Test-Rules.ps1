#requires -Version 7.0
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$output=Join-Path $projectRoot 'Saved\RuleTests'
New-Item -ItemType Directory -Force $output | Out-Null
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$visualStudio=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$visualStudio){throw 'Visual Studio C++ build tools are required.'}
$vcvars=Join-Path $visualStudio 'VC\Auxiliary\Build\vcvars64.bat'
$source=Join-Path $projectRoot 'Tests\RulesTest.cpp'
$program=Join-Path $output 'RulesTest.exe'
$object=Join-Path $output 'RulesTest.obj'
$batch=Join-Path $output 'RunRules.cmd'
@("@echo off", ('call "'+$vcvars+'" >nul'), 'if errorlevel 1 exit /b 1', ('cl /nologo /EHsc /std:c++20 /O2 /Fe:"'+$program+'" /Fo:"'+$object+'" "'+$source+'"'), 'if errorlevel 1 exit /b 1', ('"'+$program+'"')) | Set-Content -LiteralPath $batch -Encoding ascii
& cmd.exe /d /c $batch
if($LASTEXITCODE -ne 0){throw 'Native C++ rule checks failed.'}
