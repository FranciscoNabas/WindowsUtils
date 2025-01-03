@{
    GUID = 'C5F7B91E-668D-46AC-9F0E-23B0A74ABCBA'
    ModuleVersion = '1.12.1'
    RootModule = 'WindowsUtils.dll'
    CompatiblePSEditions = @(
        'Desktop',
        'Core'
    )
    Author = 'Francisco Nabas'
    Copyright = '(c) Francisco Nabas 2025.'
    Description = 'This module contains tools to facilitate the administration of Windows computers.'
    PowerShellVersion = '5.1'
    NestedModules = @('WindowsUtils.psm1')
    RequiredAssemblies = @(
        'WindowsUtils.dll',
        'WuCore.dll',
        'System.Security.Principal.Windows.dll',
        'System.Security.AccessControl.dll',
        'System.ServiceProcess.ServiceController.dll'
    )
    TypesToProcess = @('WindowsUtils.Types.ps1xml')
    FormatsToProcess = @('WindowsUtils.Format.ps1xml')
    FunctionsToExport = @(
        'Get-InstalledDotnet'
    )
    CmdletsToExport = @(
        'Get-ComputerSession'
        'Get-ObjectHandle'
        'Get-ErrorString'
        'Get-LastWin32Error'
        'Get-MsiProperties'
        'Get-RemoteMessageOptions'
        'Get-ResourceMessageTable'
        'Send-RemoteMessage'
        'Send-Click'
        'Disconnect-Session'
        'Remove-Service'
        'Get-ServiceSecurity'
        'New-ServiceAccessRule'
        'New-ServiceAuditRule'
        'Set-ServiceSecurity'
        'Expand-Cabinet'
        'Start-Tcping'
        'Start-ProcessAsUser'
        'Get-NetworkFile'
        'Close-NetworkFile'
        'New-Cabinet'
        'Test-Port'
        'Get-ProcessModule'
        'Suspend-Process'
        'Resume-Process'
        'Get-ErrorInformation'
        'Get-MsiSummaryInfo'
        'Get-MsiTableInfo'
        'Get-MsiTableData'
        'Invoke-MsiQuery'
        'Get-NetworkStatistics'
    )
    AliasesToExport = @(
        'gethandle'
        'disconnect'
        'psfile'
        'getnetfile'
        'closenetfile'
        'testport'
        'getdotnet'
        'listdlls'
        'suspend'
        'resume'
        'err'
        'gerrmess'
        'imsisql'
        'getnetstat'
    )
    PrivateData = @{
        PSData = @{
            LicenseUri    = 'https://github.com/FranciscoNabas/WindowsUtils/blob/main/LICENSE'
            ProjectUri    = 'https://github.com/FranciscoNabas/WindowsUtils'
            ReleaseNotes  = 'https://github.com/FranciscoNabas/WindowsUtils/blob/main/CHANGELOG.md'
        }
    }
}