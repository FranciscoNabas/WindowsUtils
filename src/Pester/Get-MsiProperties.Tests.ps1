Describe 'Get-MsiProperties' {
    It 'Gets the properties from a MSI file' {
        $result = Get-MsiProperties -Path '.\src\Pester\PesterInstallerTest.msi'
        $result.ALLUSERS | Should -Be '1'
        $result.ARPHELPLINK | Should -Be 'https://github.com/FranciscoNabas/WindowsUtils/blob/main/README.md'
        $result.ARPPRODUCTICON | Should -Be 'WindowsUtilsIcon'
        $result.ARPURLINFOABOUT | Should -Be 'https://github.com/FranciscoNabas/WindowsUtils'
        $result.Manufacturer | Should -Be 'Francisco Nabas Labs'
        $result.ProductCode | Should -Be '{C5F7B91E-668D-46AC-9F0E-23B0A74ABCBA}'
        $result.ProductLanguage | Should -Be '1033'
        $result.ProductName | Should -Be 'WindowsUtils, the preposterous PowerShell module.'
        $result.ProductVersion | Should -Be '6.6.6'
        $result.SecureCustomProperties | Should -Be 'WIX_DOWNGRADE_DETECTED;WIX_UPGRADE_DETECTED'
        $result.SUPERTITSPROPERTY | Should -Be 'BOOBIES'
        $result.UpgradeCode | Should -Be '{DDBFF83C-2431-4FE5-8FE8-086484B87B0E}'
    }
}