#requires -Version 7.0

function Assert-SemanticVersion([string]$Version) {
    $pattern='\A(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)(?:-(?:0|[1-9][0-9]*|[0-9]*[A-Za-z-][0-9A-Za-z-]*)(?:\.(?:0|[1-9][0-9]*|[0-9]*[A-Za-z-][0-9A-Za-z-]*))*)?(?:\+[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?\z'
    if($Version -cnotmatch $pattern){throw "Invalid SemVer: '$Version'."}
}

function Get-ProjectVersion([string]$Path) {
    $versions=[Collections.Generic.List[string]]::new()
    $inSection=$false
    foreach($line in [IO.File]::ReadAllLines($Path)){
        if($line -match '^\s*\[([^\]]+)\]\s*$'){
            $inSection=$Matches[1] -eq '/Script/EngineSettings.GeneralProjectSettings'
            continue
        }
        if(!$inSection){continue}
        if($line -match '^\s*ProjectVersion\s*=(.*)$'){$versions.Add($Matches[1].Trim())}
    }
    if($versions.Count -ne 1){throw 'Expected exactly one ProjectVersion in GeneralProjectSettings.'}
    Assert-SemanticVersion $versions[0]
    return $versions[0]
}

function Get-BuildVersion([string]$Version,[string]$BuildNumber) {
    Assert-SemanticVersion $Version
    $number=0L
    if($BuildNumber -cnotmatch '\A[1-9][0-9]*\z' -or ![long]::TryParse($BuildNumber,[ref]$number)){
        throw 'Build number must be a positive Int64 without leading zeroes.'
    }
    if($Version.Contains('+')){return "$Version.build.$BuildNumber"}
    return "$Version+build.$BuildNumber"
}

# The caller holds the project build lock; failed builds retain their reserved number.
function Reserve-BuildNumber([string]$Path) {
    $counterPath=[IO.Path]::GetFullPath($Path)
    $previous=0L
    if([IO.File]::Exists($counterPath)){
        $state=[IO.File]::ReadAllText($counterPath)
        if($state -cnotmatch '\A(?:0|[1-9][0-9]*)(?:\r?\n)?\z' -or
            ![long]::TryParse($state.TrimEnd("`r","`n"),[ref]$previous)){
            throw 'Invalid build counter; expected a nonnegative Int64 without leading zeroes.'
        }
    }
    if($previous -eq [long]::MaxValue){throw 'Build counter overflow.'}
    $next=($previous+1L).ToString([Globalization.CultureInfo]::InvariantCulture)
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($counterPath)) | Out-Null
    $temporaryPath=$counterPath+'.'+[Guid]::NewGuid().ToString('N')+'.tmp'
    try {
        [IO.File]::WriteAllText($temporaryPath,$next+"`n",[Text.UTF8Encoding]::new($false))
        [IO.File]::Move($temporaryPath,$counterPath,$true)
    } finally {
        if([IO.File]::Exists($temporaryPath)){[IO.File]::Delete($temporaryPath)}
    }
    return $next
}
