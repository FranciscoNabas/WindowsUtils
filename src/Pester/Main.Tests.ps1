Import-Module .\Release\WindowsUtils\WindowsUtils.psd1

$testPath = "$PSScriptRoot"
Invoke-Pester -Path @(
    "$testPath\Get-ComputerSession.Tests.ps1"
    "$testPath\Get-ErrorString.Tests.ps1"
    "$testPath\Get-MsiProperties.Tests.ps1"
    "$testPath\Get-ObjectHandle.Tests.ps1"
    "$testPath\Get-ResourceMessageTable.Tests.ps1"
    "$testPath\Remove-Service.Tests.ps1"
)