<#
    .SYNOPSIS

        Returns installed .NET version information.

    .DESCRIPTION

        This function returns .NET version information for the current, or remote computer.
        Uses the registry to retrieve .NET Framework information, and the runtime to return .NET information.
        It also returns the CLR version, and installed patches.

    .PARAMETER ComputerName

        A list of computer names. The function first tries Remote Registry, and falls back to PS Remote, in case the Remote Registry service is not running.

    .PARAMETER Edition

        The edition(s) to get version information. Default is 'All'.

    .PARAMETER Credential

        Optional credential.

    .PARAMETER IncludeUpdate

        Includes .NET patching information. This parameter is only allowed with .NET Framework.

    .EXAMPLE

        Get-InstalledDotNetInformation -ComputerName MYCOMPUTER1.contoso.com, MYCOMPUTER2.contoso.com -Edition 'DotnetFramework' -Credential (Get-Credential) -IncludeUpdate

    .EXAMPLE

        Get-InstalledDotNetInformation -Edition 'Dotnet' 

    .NOTES

        This section will keep a brief history, as new versions gets released.

        # 2023-JUN-24 - v1.0:

            .NET 8.0.0-preview.5
            .NET Framework 4.8.1

        Scripted by: Francisco Nabas
        Version: 1.0

        This software is part of the WindowsUtils module, distributed under the MIT license.
        This software is, and will always be free.

    .LINK

        https://learn.microsoft.com/en-us/dotnet/framework/migration-guide/how-to-determine-which-versions-are-installed
        https://learn.microsoft.com/en-us/dotnet/framework/migration-guide/how-to-determine-which-net-framework-updates-are-installed
        https://learn.microsoft.com/en-us/dotnet/core/install/how-to-detect-installed-versions?pivots=os-windows
#>
function Get-InstalledDotnet {

    [CmdletBinding()]
    param (
        [Parameter(HelpMessage = 'The computer name(s) to retrieve the information from. Default is the current computer.')]
        [ValidateNotNullOrEmpty()]
        [string[]]$ComputerName,

        [Parameter(HelpMessage = "The .NET edition. Accepted values are 'All', 'Dotnet', and 'DotnetFramework'. Default is 'All'.")]
        [ValidateSet('All', 'Dotnet', 'DotnetFramework')]
        [string]$Edition = 'All',

        [Parameter(HelpMessage = 'The credentials to query information with.')]
        [pscredential]$Credential,

        [Parameter(HelpMessage = 'Includes .NET installed updates.')]
        [ValidateScript({
            if ($Edition -eq 'Dotnet') {
                throw [ArgumentException]::new("'IncludeUpdate' is only allowed with .NET Framework.")
            }

            return $true
        })]
        [switch]$InlcudeUpdate
    )

    Begin {
        $ErrorActionPreference = 'Stop'
                
        #region Initial setup
        [System.Collections.Generic.List[WindowsUtils.DotNetVersionInfo]]$remoteVersionInfo = @()
        [System.Management.Automation.Runspaces.PSSession]$psSession = $null
        [System.Collections.Generic.List[WindowsUtils.Registry.RegistryManager]]$regHandleList = @()
        #endregion
    }

    Process {
        if ($ComputerName) {
            foreach ($computer in $ComputerName) {
                if ($Edition -eq 'DotnetFramework' -or $Edition -eq 'All') {
                    if ($Credential) { $regManager = [WindowsUtils.Registry.RegistryManager]::new($computer, $Credential, [Microsoft.Win32.RegistryHive]::LocalMachine) }
                    else { $regManager = [WindowsUtils.Registry.RegistryManager]::new($computer, [Microsoft.Win32.RegistryHive]::LocalMachine) }
                    [void]$regHandleList.Add($regManager)
                }
                
                $mainSplat = @{
                    Session = ([ref]$psSession)
                    ComputerName = $computer
                }
                if ($Credential) { $mainSplat.Credential = $Credential }

                switch ($Edition) {
                    'Dotnet' {
                        $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                        if ($result.Error) {
                            if ($result.Result.Exception.Message -notlike 'The term*is not recognized*') {
                                Write-Error -ErrorRecord $result.Result
                            }
                        }
                        else {
                            [version]$version = $null
                            if ([version]::TryParse($result.Result, [ref]$version)) {
                                [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, $version, 'Core'))
                            }
                        }
                    }
                    'DotnetFramework' {
                        $result = Get-RemoteDotNetFFVersionInfo @mainSplat -RegistryManager ([ref]$regManager)
                        if (![string]::IsNullOrEmpty($result.Version) -and $result.Release -ne 0) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new($result.Release, [version]$result.Version, 'FullFramework', $computer))
                        }
                        foreach ($legacyVersion in $result.Legacy) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$legacyVersion, 'FullFramework', $computer))
                        }
                    }
                    Default {
                        $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                        if ($result.Error) {
                            if ($result.Result.Exception.Message -notlike 'The term*is not recognized*') {
                                Write-Error -ErrorRecord $result.Result
                            }
                        }
                        else {
                            [version]$version = $null
                            if ([version]::TryParse($result.Result, [ref]$version)) {
                                [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, $version, 'Core'))
                            }
                        }

                        $resultFf = Get-RemoteDotNetFFVersionInfo @mainSplat -RegistryManager ([ref]$regManager)
                        if (![string]::IsNullOrEmpty($resultFf.Version) -and $resultFf.Release -ne 0) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new($resultFf.Release, [version]$resultFf.Version, 'FullFramework', $computer))
                        }
                        foreach ($legacyVersion in $resultFf.Legacy) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$legacyVersion, 'FullFramework', $computer))
                        }
                    }
                }
            }
        }
        else {
            if ($Edition -eq 'DotnetFramework' -or $Edition -eq 'All') {
                if ($Credential) { $regManager = [WindowsUtils.Registry.RegistryManager]::new($Credential, [Microsoft.Win32.RegistryHive]::LocalMachine) }
                else { $regManager = [WindowsUtils.Registry.RegistryManager]::new([Microsoft.Win32.RegistryHive]::LocalMachine) }
                [void]$regHandleList.Add($regManager)
            }

            $mainSplat = @{ Session = ([ref]$psSession) }
            if ($Credential) { $mainSplat.Credential = $Credential }

            switch ($Edition) {
                'Dotnet' {
                    $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                    if ($result.Error) {
                        if ($result.Result.Exception.Message -notlike 'The term*is not recognized*') {
                            Write-Error -ErrorRecord $result.Result
                        }
                    }
                    else {
                        [version]$version = $null
                        if ([version]::TryParse($result.Result, [ref]$version)) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, $version, 'Core'))
                        }
                    }
                }
                'DotnetFramework' {
                    $result = Get-RemoteDotNetFFVersionInfo @mainSplat -RegistryManager ([ref]$regManager)
                    if (![string]::IsNullOrEmpty($result.Version) -and $result.Release -ne 0) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new($result.Release, [version]$result.Version, 'FullFramework'))
                    }
                    foreach ($legacyVersion in $result.Legacy) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$legacyVersion, 'FullFramework'))
                    }
                }
                Default {
                    $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                    if ($result.Error) {
                        if ($result.Result.Exception.Message -notlike 'The term*is not recognized*') {
                            Write-Error -ErrorRecord $result.Result
                        }
                    }
                    else {
                        [version]$version = $null
                        if ([version]::TryParse($result.Result, [ref]$version)) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, $version, 'Core'))
                        }
                    }

                    $resultFf = Get-RemoteDotNetFFVersionInfo @mainSplat -RegistryManager ([ref]$regManager)
                    if (![string]::IsNullOrEmpty($resultFf.Version) -and $resultFf.Release -ne 0) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new($resultFf.Release, [version]$resultFf.Version, 'FullFramework'))
                    }
                    foreach ($legacyVersion in $resultFf.Legacy) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$legacyVersion, 'FullFramework'))
                    }
                }
            }
        }

        if ($InlcudeUpdate) {
            [System.Collections.Generic.List[WindowsUtils.DotNetInstalledUpdateInfo]]$patchInfo = @()
            if ($ComputerName) {
                foreach ($computer in $ComputerName) {
                    $mainSplat = @{
                        ComputerName = $computer
                        RegistryManager = ([ref]$regHandleList.Where({ $_.ComputerName -eq $computer }))
                        Session = ([ref]$psSession)
                    }
                    if ($Credential) { $mainSplat.Credential = $Credential }
                    
                    foreach ($info in (Get-DotNetInstalledPatches @mainSplat)) {
                        [void]$patchInfo.Add($info)
                    }
                }
            }
            else {
                $mainSplat = @{
                    RegistryManager = ([ref]$regManager)
                    Session = ([ref]$psSession)
                }
                if ($Credential) { $mainSplat.Credential = $Credential }
                
                foreach ($info in (Get-DotNetInstalledPatches @mainSplat)) {
                    [void]$patchInfo.Add($info)
                }
            }

            Write-Output ([PSCustomObject]@{
                VersionInfo = $remoteVersionInfo
                InstalledUpdates = $patchInfo
            })
        }
        else {
            Write-Output $remoteVersionInfo
        }
    }

    End {
        foreach ($handle in $regHandleList) {
            $handle.Dispose()
        }
    }
}

function Get-DotNetInstalledPatches {

    param(
        [ref]$RegistryManager,
        [string]$ComputerName,
        [pscredential]$Credential,
        [ref]$Session
    )

    $fallback = $false
    [System.Collections.Generic.List[WindowsUtils.DotNetInstalledUpdateInfo]]$patchInfo = @()
    try {
        foreach ($mainVersion in $RegistryManager.Value.GetRegistrySubKeyNames('SOFTWARE\WOW6432Node\Microsoft\Updates').Where({ $_ -like  '*.NET Framework*'})) {
            $result = $RegistryManager.Value.GetRegistrySubKeyNames("SOFTWARE\WOW6432Node\Microsoft\Updates\$mainVersion")
            [void]$patchInfo.Add([WindowsUtils.DotNetInstalledUpdateInfo]::new($mainVersion, $result))
        }
    }
    catch {
        if ($_.Exception.InnerException.NativeErrorCode -eq 53) {
            $fallback = $true
        }
        else {
            throw $_
        }
    }

    if ($fallback) {
        if ([string]::IsNullOrEmpty($ComputerName)) {
            $isNewPsDrive = $false
            if ($Credential) {
                $psDrive = New-PSDrive -Name 'HKLMImp' -PSProvider 'Registry' -Root 'HKEY_LOCAL_MACHINE' -Credential $Credential
                $isNewPsDrive = $true
            }
            else {
                $psDrive = Get-PSDrive -Name 'HKLM'
            }
            
            try {
                foreach ($subkey in (Get-ChildItem -Path 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Updates\' | Where-Object { $_.Name -like  '*.NET Framework*'})) {
                    [void]$patchInfo.Add([WindowsUtils.DotNetInstalledUpdateInfo]::new(
                        $subkey,
                        (Get-ChildItem -Path "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Updates\$($subkey.PSChildName)").PSChildName
                    ))
                }
            }
            catch {
                throw $_
            }
            finally {
                if ($isNewPsDrive) {
                    Remove-PSDrive -Name $psDrive.Name -Force
                }
            }
        }
        else {
            if (!$Session.Value.Runspace -or $Session.Value.Runspace.RunspaceStateInfo.State -ne 'Opened') {
                if ($Session.Value.Runspace) { $Session.Value.Runspace.CloseAsync() }
                
                $remoteSessSplat = @{ ComputerName = $ComputerName }
                if ($Credential) { $remoteSessSplat.Credential = $Credential }
        
                $Session.Value = New-PSSession @remoteSessSplat
            }

            try {
                $result = Invoke-Command -Session $Session.Value -ScriptBlock {
                    $ErrorActionPreference = 'SilentlyContinue'
                
                    foreach ($subkey in (Get-ChildItem -Path 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Updates\' | Where-Object { $_.Name -like  '*.NET Framework*'})) {
                        [pscustomobject]@{
                            Version = $subkey
                            Patches = (Get-ChildItem -Path "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Updates\$($subkey.PSChildName)").PSChildName
                        }
                    }
                }

                foreach ($info in $result) {
                    [void]$patchInfo.Add([WindowsUtils.DotNetInstalledUpdateInfo]::new($ComputerName, $info.Version, $info.Patches))
                }
            }
            catch {
                throw $_
            }
        }
    }

    return $patchInfo
}

function Get-RemoteDotNetCoreVersionInfo {

    param(
        [string]$ComputerName,
        [pscredential]$Credential,
        [ref]$Session
    )

    $isError = $false
    $invCmdSplat = @{
        ScriptBlock = {
            $result = ''
            try {
                if (Test-Path -Path "$env:ProgramFiles\dotnet\dotnet.exe" -PathType 'Leaf') {
                    $result = & "$env:ProgramFiles\dotnet\dotnet.exe" --version
                }
                else {
                    if (Test-Path -Path "${env:ProgramFiles(x86)}\dotnet\dotnet.exe" -PathType 'Leaf') {
                        $result = & "$env:ProgramFiles\dotnet\dotnet.exe" --version
                    }
                    else {
                        # Trying from the PATH.
                        $result = & 'dotnet.exe' --version
                    }
                }
            }
            catch {
                $result = $_
                $isError = $true
            }

            return [pscustomobject]@{ Result = $result; Error = $isError }
        }
    }
    
    if (![string]::IsNullOrEmpty($ComputerName)) {
        if (!$Session.Value.Runspace -or $Session.Value.Runspace.RunspaceStateInfo.State -ne 'Opened') {
            if ($Session.Value.Runspace) { $Session.Value.Runspace.CloseAsync() }
            
            $remoteSessSplat = @{ ComputerName = $ComputerName }
            if ($Credential) { $remoteSessSplat.Credential = $Credential }
    
            $Session.Value = New-PSSession @remoteSessSplat
        }
        $invCmdSplat.Session = $Session.Value
    }

    return Invoke-Command @invCmdSplat
}

function Get-RemoteDotNetFFVersionInfo {
    
    param(
        [ref]$RegistryManager,
        [string]$ComputerName,
        [pscredential]$Credential,
        [ref]$Session
    )

    $output = [PSCustomObject]@{
        ComputerName = $ComputerName
        Version = $null
        Release = 0
        Legacy = [System.Collections.Generic.List[string]]::new()
    }

    # Attempting to get installed versions greater than 4.5.
    try {
        $getVerSplat = @{
            RegistryManager = $RegistryManager
            SubKey = 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full'
            ValueName = @('Release', 'Version')
            ComputerName = $ComputerName
            Credential = $Credential
            Session = $Session
        }

        try {
            $mainVersionInfo = Get-RegistryValueWithWinrmFallback @getVerSplat
            $release = $mainVersionInfo[0]
            $versionText = $mainVersionInfo[1]
        }
        catch {
            $getVerSplat.ValueName = @('Release')
            $mainVersionInfo = Get-RegistryValueWithWinrmFallback @getVerSplat
            $release = $mainVersionInfo[0]
        }

        if ([string]::IsNullOrEmpty($versionText)) {
            $ffVersionInfo = [WindowsUtils.DotNetVersionInfo]::GetInfoFromRelease($release)
            $output.Release = $release
            $output.Version = $ffVersionInfo.Version
        }
        else {
            $output.Release = $release
            $output.Version = $versionText
        }
    }
    catch {
        if (!($_.Exception.InnerException.NativeErrorCode -eq 2)) {
            Write-Error -Exception $_.Exception
        }
    }

    #region Legacy versions
    foreach ($subKey in $RegistryManager.Value.GetRegistrySubKeyNames('SOFTWARE\Microsoft\NET Framework Setup\NDP\').Where({ $_ -ne 'CDF' -and $_ -ne 'v4.0' })) {
        $getVerSplat = @{
            RegistryManager = $RegistryManager
            Session = $Session
        }
        
        switch ($subKey) {
            'v3.0' {
                $getVerSplat.SubKey = 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v3.0\Setup'
                $getVerSplat.ValueName = @('InstallSuccess', 'Version')
                $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                
                if ($result[0] -eq 1) {
                    if ([string]::IsNullOrEmpty($result[1])) {
                        $output.Legacy.Add('v3.0')
                    }
                    else {
                        $output.Legacy.Add($result[1])
                    }
                }
            }
            'v4' {
                foreach ($versionProfile in @('Client', 'Full')) {
                    $getVerSplat.SubKey = "SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\$versionProfile"
                    $getVerSplat.ValueName = @('Install', 'Version')
                    $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                
                    if ($result[0] -eq 1) {
                        if ([string]::IsNullOrEmpty($result[1])) {
                            $output.Legacy.Add("v4-$versionProfile")
                        }
                        else {
                            if ([version]$result[1] -lt [version]'4.5') {
                                $output.Legacy.Add("$($result[1])-$versionProfile")
                            }
                        }
                    }
                }
            }
            Default {
                $getVerSplat.SubKey = "SOFTWARE\Microsoft\NET Framework Setup\NDP\$subKey"
                $getVerSplat.ValueName = @('Install', 'Version')
                $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                
                if ($result[0] -eq 1) {
                    if ([string]::IsNullOrEmpty($result[1])) {
                        $output.Legacy.Add($subKey)
                    }
                    else {
                        $output.Legacy.Add($result[1])
                    }
                }
            }
        }
    }
    #endregion

    return $output
}

function Get-RegistryValueWithWinrmFallback {

    [CmdletBinding()]
    param(
        [ref]$RegistryManager,
        [string]$SubKey,
        [string[]]$ValueName,
        [string]$ComputerName,
        [pscredential]$Credential,
        [ref]$Session
    )

    try {
        $result = $RegistryManager.Value.GetRegistryValueList($SubKey, $ValueName)
        return $result
    }
    catch {
        if ($_.Exception.InnerException.NativeErrorCode -eq 53) {
            Write-Verbose 'Failed with native error code 53.'
            $fallback = $true
        }
        else {
            throw $_
        }
    }

    if ($fallback) {
        
        if ([string]::IsNullOrEmpty($ComputerName)) {
            $isNewDrive = $false
            try {
                Write-Verbose 'Attempting using WinRM on local computer.'
                switch ($Hive) {
                    'ClassesRoot' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_CLASSES_ROOT'; DriveName = 'HKCR' } }
                    'CurrentUser' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_CURRENT_USER'; DriveName = 'HKCU' } }
                    'LocalMachine' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_LOCAL_MACHINE'; DriveName = 'HKLM' } }
                    'Users' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_USERS'; DriveName = 'HKU' } }
                    'PerformanceData' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_PERFORMANCE_DATA'; DriveName = 'HKPD' } }
                    'CurrentConfig' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_CURRENT_CONFIG'; DriveName = 'HKCC' } }
                }
                
                if ($Credential) {
                    $psDrive = New-PSDrive -Name "$($hiveData.DriveName)Imp" -PSProvider 'Registry' -Root $hiveData.HiveName -Credential $Credential
                    $isNewDrive = $true
                }
                else {
                    $psDrive = Get-PSDrive -PSProvider 'Registry' | Where-Object { $_.Root -eq $hiveData.HiveName }
                    if (!$psDrive) {
                        $psDrive = New-PSDrive -Name $hiveData.DriveName -PSProvider 'Registry' -Root $hiveData.HiveName
                        $isNewDrive = $true
                    }
                }
        
                return Get-ItemPropertyValue -LiteralPath "$($psDrive.Name):\$SubKey" -Name $ValueName
            }
            catch {
                throw $_
            }
            finally {
                if ($isNewDrive) {
                    Remove-PSDrive -Name $hiveData.DriveName -Force
                }
            }
        }
        else {
            try {
                if (!$Session.Value.Runspace -or $Session.Value.Runspace.RunspaceStateInfo.State -ne 'Opened') {
                    if ($Session.Value.Runspace) { $Session.Value.Runspace.CloseAsync() }
                    
                    $remoteSessSplat = @{ ComputerName = $ComputerName }
                    if ($Credential) { $remoteSessSplat.Credential = $Credential }

                    $Session.Value = New-PSSession @remoteSessSplat
                }

                Write-Verbose "Attempting using WinRM on '$ComputerName'."
                $result = Invoke-Command -Session $Session.Value -ScriptBlock {
                    $isNewDrive = $false
                    try {
                        switch ($Using:Hive) {
                            'ClassesRoot' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_CLASSES_ROOT'; DriveName = 'HKCR' } }
                            'CurrentUser' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_CURRENT_USER'; DriveName = 'HKCU' } }
                            'LocalMachine' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_LOCAL_MACHINE'; DriveName = 'HKLM' } }
                            'Users' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_USERS'; DriveName = 'HKU' } }
                            'PerformanceData' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_PERFORMANCE_DATA'; DriveName = 'HKPD' } }
                            'CurrentConfig' { $hiveData = [pscustomobject]@{ HiveName = 'HKEY_CURRENT_CONFIG'; DriveName = 'HKCC' } }
                        }
                        
                        $psDrive = Get-PSDrive -PSProvider 'Registry' | Where-Object { $_.Root -eq $hiveData.HiveName }
                        if (!$psDrive) {
                            $psDrive = New-PSDrive -Name $hiveData.DriveName -PSProvider 'Registry' -Root $hiveData.HiveName
                            $isNewDrive = $true
                        }
        
                        Get-ItemPropertyValue -LiteralPath "$($hiveData.DriveName):\$Using:SubKey" -Name $Using:ValueName
                    }
                    catch {
                        throw $_
                    }
                    finally {
                        if ($isNewDrive) {
                            Remove-PSDrive -Name $hiveData.DriveName -Force
                        }
                    }
                }
    
                if ($result.GetType() -eq [System.Management.Automation.ErrorRecord]) {
                    throw $result
                }
    
                return $result
            }
            catch {
                throw $_
            }
        }
    }
}