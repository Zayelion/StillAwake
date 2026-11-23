#requires -Version 7.0
param([string]$ResultsDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
. (Join-Path $PSScriptRoot 'Version.ps1')
$checks=[Collections.Generic.List[string]]::new()
function Check([bool]$Valid,[string]$Name){
    if(!$Valid){throw "FAIL: $Name"}
    $script:checks.Add($Name)
}
function Expect-Failure([scriptblock]$Action,[string]$Message,[string]$Name){
    $failed=$false
    try { & $Action | Out-Null }
    catch {
        if($_.Exception.Message -notlike "*$Message*"){throw}
        $failed=$true
    }
    Check $failed $Name
}
if(!$ResultsDirectory){$ResultsDirectory=Join-Path $root ('.tmp/version-'+[Guid]::NewGuid().ToString('N'))}
$resultsPath=[IO.Path]::GetFullPath($ResultsDirectory)
[IO.Directory]::CreateDirectory($resultsPath) | Out-Null
$settingsPath=Join-Path $resultsPath 'DefaultGame.ini'
$section='[/Script/EngineSettings.GeneralProjectSettings]'

foreach($version in @('0.0.0','0.3.0','1.2.3-alpha.1','1.2.3-0','1.2.3-01a','1.2.3-x-y-z.--',
    '1.2.3+001','1.2.3-alpha.1+sha.abc-123')){
    Check ((Get-BuildVersion $version '12').StartsWith($version)) "Valid SemVer $version"
}
foreach($version in @('','1','1.2','v1.2.3','01.2.3','1.02.3','1.2.03','1.2.3-01','1.2.3-alpha.01',
    '1.2.3-','1.2.3+','1.2.3-alpha..1','1.2.3+a..b','1.2.3+a_b',"1.2.3`n",' 1.2.3','1.2.3 ',
    ([char]0x0661).ToString()+'.2.3','1.2.3-'+([char]0x0661).ToString())){
    Expect-Failure {Get-BuildVersion $version '1'} 'SemVer' "Reject invalid SemVer '$version'"
}
Check ((Get-BuildVersion '0.3.0' '1') -ceq '0.3.0+build.1') 'Compose normal build version'
Check ((Get-BuildVersion '0.3.0-rc.2' '12') -ceq '0.3.0-rc.2+build.12') 'Preserve prerelease'
Check ((Get-BuildVersion '0.3.0+sha.abc' '12') -ceq '0.3.0+sha.abc.build.12') 'Append existing metadata'
Check ((Get-BuildVersion '0.3.0-rc.2+001' '12') -ceq '0.3.0-rc.2+001.build.12') 'Preserve prerelease and metadata'
Check ((Get-BuildVersion '0.3.0' ([long]::MaxValue.ToString())) -ceq '0.3.0+build.9223372036854775807') 'Accept maximum build number'
foreach($number in @('','0','01','-1','+1','1.0','1 ',"1`n",([char]0x0661).ToString(),'9223372036854775808')){
    Expect-Failure {Get-BuildVersion '0.3.0' $number} 'Build number' "Reject invalid build number '$number'"
}

[IO.File]::WriteAllText($settingsPath,"[Other]`nProjectVersion=9.9.9`n$section`nProjectVersion=0.3.0-rc.1+001`n")
$settings=[IO.File]::ReadAllText($settingsPath)
Check ((Get-ProjectVersion $settingsPath) -ceq '0.3.0-rc.1+001') 'Read version only from project settings'
Check ([IO.File]::ReadAllText($settingsPath) -ceq $settings) 'Reading version preserves settings bytes'
foreach($settings in @("$section`n",'[Other]'+"`nProjectVersion=0.3.0`n",
    "$section`nProjectVersion=0.3.0`nProjectVersion=0.4.0`n",
    "$section`nProjectVersion=0.3.0`n$section`nProjectVersion=0.4.0`n")){
    [IO.File]::WriteAllText($settingsPath,$settings)
    Expect-Failure {Get-ProjectVersion $settingsPath} 'exactly one ProjectVersion' 'Reject missing or duplicate project version'
}
[IO.File]::WriteAllText($settingsPath,"$section`nProjectVersion=0.03.0`n")
Expect-Failure {Get-ProjectVersion $settingsPath} 'SemVer' 'Reject invalid configured SemVer'

$counterPath=Join-Path $resultsPath ('Counter-'+[Guid]::NewGuid().ToString('N')+'/BuildNumber.txt')
Check ((Reserve-BuildNumber $counterPath) -ceq '1') 'First reservation creates build one'
Check ((Reserve-BuildNumber $counterPath) -ceq '2') 'Second reservation increases monotonically'
Check ([IO.File]::ReadAllText($counterPath) -ceq "2`n") 'Counter persists the reserved number'
# A failed build performs no rollback; the next reservation skips its consumed number.
Check ((Reserve-BuildNumber $counterPath) -ceq '3') 'Reservation remains consumed without build success'
foreach($state in @('0',"4`r`n",'9223372036854775806')){
    [IO.File]::WriteAllText($counterPath,$state)
    $expected=([long]$state+1L).ToString([Globalization.CultureInfo]::InvariantCulture)
    Check ((Reserve-BuildNumber $counterPath) -ceq $expected) "Read valid counter '$state'"
}
foreach($state in @('','01','-1','+1','1.0',' 1',"1`n`n",([char]0x0661).ToString(),'9223372036854775808')){
    [IO.File]::WriteAllText($counterPath,$state)
    Expect-Failure {Reserve-BuildNumber $counterPath} 'Invalid build counter' "Reject corrupt counter '$state'"
    Check ([IO.File]::ReadAllText($counterPath) -ceq $state) 'Invalid counter remains untouched'
}
[IO.File]::WriteAllText($counterPath,[long]::MaxValue.ToString())
Expect-Failure {Reserve-BuildNumber $counterPath} 'overflow' 'Reject exhausted counter'
Check ([IO.File]::ReadAllText($counterPath) -ceq [long]::MaxValue.ToString()) 'Overflow leaves counter untouched'
[IO.File]::WriteAllText($counterPath,'7')
$counterLock=[IO.File]::Open($counterPath,'Open','Read','Read')
try {
    Expect-Failure {Reserve-BuildNumber $counterPath} 'Move' 'Locked counter rejects reservation'
} finally { $counterLock.Dispose() }
Check ([IO.File]::ReadAllText($counterPath) -ceq '7') 'Failed atomic move preserves counter'
Check (@(Get-ChildItem -LiteralPath (Split-Path $counterPath) -Filter '*.tmp').Count -eq 0) 'Reservations leave no temporary files'
@{passed=$true;checks=$checks.Count;cases=$checks} | ConvertTo-Json -Depth 4 |
    Set-Content -LiteralPath (Join-Path $resultsPath 'version-result.json') -Encoding utf8
Write-Host "PASS: $($checks.Count) version checks. Report: $resultsPath"
