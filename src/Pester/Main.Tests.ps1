Push-Location -Path $PSScriptRoot
Import-Module .\..\..\Release\WindowsUtils\WindowsUtils.psd1
Add-Type -Path .\Utilities\WuPesterHelper\WuPesterHelper.dll

Invoke-Pester -Path @(
    "$PSScriptRoot\Commands\TerminalServices\Get-ComputerSession.Tests.ps1"
    "$PSScriptRoot\Commands\Utilities\Get-ErrorString.Tests.ps1"
    "$PSScriptRoot\Commands\Installer\Get-MsiProperties.Tests.ps1"
    "$PSScriptRoot\Commands\ProcessAndThread\Get-ObjectHandle.Tests.ps1"
    "$PSScriptRoot\Commands\Utilities\Get-ResourceMessageTable.Tests.ps1"
    "$PSScriptRoot\Commands\Services\Remove-Service.Tests.ps1"
    "$PSScriptRoot\Commands\Containers\Expand-Cabinet.Tests.ps1"
) -Verbose

Pop-Location