<#
    .SYNOPSIS

        Returns installed .NET version information.

    .DESCRIPTION

        This function returns .NET version information for the current, or remote computer.
        Uses the registry to retrieve .NET Framework information, and the runtime to return .NET information.
        It also returns the CLR version, and installed patches.

    .PARAMETER ComputerName


    .EXAMPLE


    .NOTES

        This section will keep a brief history, as new versions gets released.

        - 2023-JUN-24 - v1.0:

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
function Get-DotNetInformation {

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
        [switch]$InlcudeUpdate
    )

    Begin {
        $ErrorActionPreference = 'Stop'
                
        #region Initial setup
        $unwrapper = [WindowsUtils.Core.Wrapper]::new()
        [System.Collections.Generic.List[WindowsUtils.DotNetVersionInfo]]$remoteVersionInfo = @()
        #endregion
    }

    Process {
        if ($ComputerName) {
            foreach ($computer in $ComputerName) {
                $mainSplat = @{ ComputerName = $computer }
                if ($Credential) { $mainSplat.Credential = $Credential }

                switch ($Edition) {
                    'Dotnet' {
                        $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                        if ($result.GetType() -eq [System.Management.Automation.ErrorRecord]) {
                            if ($_.Exception.Message -notlike 'The therm*is not recognized*') {
                                Write-Error -ErrorRecord $result
                            }
                        }
                        elseif (![string]::IsNullOrEmpty($result.Result)) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$result.Result, 'Core', $computer))
                        }
                    }
                    'DotnetFramework' {
                        $result = Get-RemoteDotNetFFVersionInfo @mainSplat -Wrapper ([ref]$unwrapper)
                        if (![string]::IsNullOrEmpty($result.Version) -and $result.Release -ne 0) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new($result.Release, [version]$result.Version, 'FullFramework', $computer))
                        }
                        foreach ($legacyVersion in $result.Legacy) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$legacyVersion, 'FullFramework', $computer))
                        }
                    }
                    Default {
                        $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                        if ($result.GetType() -eq [System.Management.Automation.ErrorRecord]) {
                            if ($_.Exception.Message -notlike 'The therm*is not recognized*') {
                                Write-Error -ErrorRecord $result
                            }
                        }
                        elseif (![string]::IsNullOrEmpty($result.Result)) {
                            [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$result.Result, 'Core', $computer))
                        }

                        $resultFf = Get-RemoteDotNetFFVersionInfo @mainSplat -Wrapper ([ref]$unwrapper)
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
            $mainSplat = @{ }
            if ($Credential) { $mainSplat.Credential = $Credential }

            switch ($Edition) {
                'Dotnet' {
                    $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                    if ($result.GetType() -eq [System.Management.Automation.ErrorRecord]) {
                        if ($_.Exception.Message -notlike 'The therm*is not recognized*') {
                            Write-Error -ErrorRecord $result
                        }
                    }
                    elseif (![string]::IsNullOrEmpty($result.Result)) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$result.Result, 'Core'))
                    }
                }
                'DotnetFramework' {
                    $result = Get-RemoteDotNetFFVersionInfo @mainSplat -Wrapper ([ref]$unwrapper)
                    if (![string]::IsNullOrEmpty($result.Version) -and $result.Release -ne 0) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new($result.Release, [version]$result.Version, 'FullFramework'))
                    }
                    foreach ($legacyVersion in $result.Legacy) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$legacyVersion, 'FullFramework'))
                    }
                }
                Default {
                    $result = Get-RemoteDotNetCoreVersionInfo @mainSplat
                    if ($result.GetType() -eq [System.Management.Automation.ErrorRecord]) {
                        if ($_.Exception.Message -notlike 'The therm*is not recognized*') {
                            Write-Error -ErrorRecord $result
                        }
                    }
                    elseif (![string]::IsNullOrEmpty($result.Result)) {
                        [void]$remoteVersionInfo.Add([WindowsUtils.DotNetVersionInfo]::new(0, [version]$result.Result, 'Core'))
                    }

                    $resultFf = Get-RemoteDotNetFFVersionInfo @mainSplat -Wrapper ([ref]$unwrapper)
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

        }
        else {
            $output = $remoteVersionInfo
        }
    }

    End {
        return $output
    }
}

function Get-RemoteDotNetCoreVersionInfo {

    param(
        [string]$ComputerName,
        [pscredential]$Credential
    )

    $invCommSplat = @{
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
                        $result = & "$env:ProgramFiles\dotnet\dotnet.exe" --version
                    }
                }
            }
            catch {
                $result = $_
            }

            return [pscustomobject]@{ Result = $result }
        }
    }
    
    if ($ComputerName) { $invCommSplat.ComputerName = $ComputerName }
    if ($Credential) { $invCommSplat.Credential = $Credential }

    return Invoke-Command @invCommSplat
}

function Get-RemoteDotNetFFVersionInfo {
    
    param(
        [string]$ComputerName,
        [pscredential]$Credential,
        [ref]$Wrapper
    )

    $output = $null

    # Attempting to get installed versions greater than 4.5.
    try {
        $getVerSplat = @{
            Wrapper = $Wrapper
            SubKey = 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full'
            ValueName = 'Release'
        }
        if (![string]::IsNullOrEmpty($ComputerName)) { $getVerSplat.ComputerName = $ComputerName }
        if ($Credential) { $getVerSplat.Credential = $Credential }

        $release = Get-RegistryValueWithWinrmFallback @getVerSplat

        $getVerSplat.ValueName = 'Version'
        try { $versionText = Get-RegistryValueWithWinrmFallback @getVerSplat }
        catch { }

        if ([string]::IsNullOrEmpty($versionText)) {
            $ffVersionInfo = [WindowsUtils.DotNetVersionInfo]::GetInfoFromRelease($release)
            $output = [PSCustomObject]@{
                ComputerName = $ComputerName
                Version = $ffVersionInfo.Version
                Release = $release
                Legacy = [System.Collections.Generic.List[string]]::new()
            }
        }
        else {
            $output = [PSCustomObject]@{
                ComputerName = $ComputerName
                Version = $versionText
                Release = $release
                Legacy = [System.Collections.Generic.List[string]]::new()
            }
        }
    }
    catch {
        if ($_.Exception.InnerException.NativeErrorCode -eq 2) {
            $output = [PSCustomObject]@{
                ComputerName = $ComputerName
                Version = $null
                Release = 0
                Legacy = [System.Collections.Generic.List[string]]::new()
            }
        }
        else {
            Write-Error -Exception $_.Exception
        }
    }

    #region Legacy versions
    foreach ($oldVersion in @(
        'v1.0'
        'v1.1.4322'
        'v2.0.50727'
        'v3.0'
        'v3.5'
        'v4'
    )) {
        switch ($oldVersion) {
            'v1.0' { $subKey = 'SOFTWARE\Microsoft\.NETFramework\Policy\v1.0\3705'; $valName = 'Install' }
            'v3.0' { $subKey = 'SOFTWARE\Microsoft\NET Framework Setup\NDP\v3.0\Setup'; $valName = 'InstallSuccess' }
            Default { $subKey = "SOFTWARE\Microsoft\NET Framework Setup\NDP\$oldVersion"; $valName = 'Install' }
        }

        try {
            if ($oldVersion -eq 'v4') {
                foreach ($version in 'Client', 'Full') {
                    $getVerSplat = @{
                        Wrapper = $Wrapper
                        SubKey = "SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\$version"
                        ValueName = 'Install'
                    }
                    if (![string]::IsNullOrEmpty($ComputerName)) { $getVerSplat.ComputerName = $ComputerName }
                    if ($Credential) { $getVerSplat.Credential = $Credential }
    
                    $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                    if ([int]$result -eq 1) {
                        $getVerSplat.ValueName = 'Version'
                        $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                        if ([version]$result -lt [version]'4.5') {
                            $output.Legacy.Add("$result-$version")
                        }
                    }
                }
            }
            else {
                $getVerSplat = @{
                    Wrapper = $Wrapper
                    SubKey = $subKey
                    ValueName = $valName
                }
                if (![string]::IsNullOrEmpty($ComputerName)) { $getVerSplat.ComputerName = $ComputerName }
                if ($Credential) { $getVerSplat.Credential = $Credential }
    
                $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                if ([int]$result -eq 1) {
                    $getVerSplat.ValueName = 'Version'
                    $result = Get-RegistryValueWithWinrmFallback @getVerSplat
                    $output.Legacy.Add("$result")
                }
            }
        }
        catch {
            continue
        }
    }

    #endregion

    return $output
}

function Get-RegistryValueWithWinrmFallback {

    param(
        [string]$ComputerName = '',
        [pscredential]$Credential,
        [ref]$Wrapper,
        [string]$SubKey,
        [string]$ValueName,
        [Microsoft.Win32.RegistryHive]$Hive = 'LocalMachine'
    )

    try {
        if ($Credential) {
            $result = $Wrapper.Value.GetRegistryValue(
                $ComputerName,
                $Credential.UserName,
                $Credential.GetNetworkCredential().Password,
                $Hive,
                $SubKey,
                $ValueName
            )
        }
        else {
            $result = $Wrapper.Value.GetRegistryValue(
                $ComputerName,
                '',
                '',
                $Hive,
                $SubKey,
                $ValueName
            )
        }

        return $result
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
            $isNewDrive = $false
            try {
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
                if ($Credential) { $remoteCmdSplat = @{ ComputerName = $ComputerName; Credential = $Credential } }
                else { $remoteCmdSplat = @{ ComputerName = $ComputerName } }
    
                $result = Invoke-Command @remoteCmdSplat -ScriptBlock {
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