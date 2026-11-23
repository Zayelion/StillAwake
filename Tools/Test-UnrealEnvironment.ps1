#requires -Version 7.0
$ErrorActionPreference='Stop'

function Test-UnrealRegistryAccess {
    param([scriptblock]$OpenKey = {
        param([string]$Path,[bool]$Writable)
        $rights=if($Writable){[System.Security.AccessControl.RegistryRights]::WriteKey}else{[System.Security.AccessControl.RegistryRights]::ReadKey}
        [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($Path,[Microsoft.Win32.RegistryKeyPermissionCheck]::Default,$rights)
    })
    $identifierPath='Software\Epic Games\Unreal Engine\Identifiers'
    try {
        $key=& $OpenKey $identifierPath $false
        if($null -ne $key){
            try {
                $value=$key.GetValue('MachineId')
                $parsed=[Guid]::Empty
                if($value -is [string] -and [Guid]::TryParse($value,[ref]$parsed)){
                    return 'PASS: Unreal can read its existing machine identity.'
                }
            } finally { $key.Dispose() }
        }
        # A first launch needs permission to create/save the ID. Open a handle only;
        # never create keys, write values, print the ID, or change permissions.
        foreach($path in @($identifierPath,'Software\Epic Games\Unreal Engine','Software\Epic Games','Software')){
            $parent=& $OpenKey $path $true
            if($null -ne $parent){
                $parent.Dispose()
                return 'PASS: Unreal has access to initialize its machine identity.'
            }
        }
        throw 'No accessible registry parent was found for Unreal initialization.'
    } catch {
        $detail=$_.Exception.GetBaseException().Message
        throw "Unreal startup is blocked by this Windows session's registry access. Required key: HKCU:\$identifierPath. $detail Run npm run build from your ordinary Windows terminal in D:\Still Awake. This check changed no registry values or permissions."
    }
}

if($MyInvocation.InvocationName -ne '.'){
    try { Test-UnrealRegistryAccess }
    catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }
}
