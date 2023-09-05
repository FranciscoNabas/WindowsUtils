BeforeAll {
    Import-Module .\Release\WindowsUtils\WindowsUtils.psd1
}

Describe 'Get-ComputerSession' {
    It 'Gets the active session' {
        $computer_session = Get-ComputerSession
        $computer_session.Count | Should -Be 1
        $computer_session.ComputerName | Should -Be $env:COMPUTERNAME
    }

    It 'Gets system sessions' {
        $computer_session = Get-ComputerSession -IncludeSystemSession
        $computer_session.Count | Should -BeGreaterThan 1
        $computer_session.Where({ [string]::IsNullOrEmpty($_.UserName) })[0].State | Should -Be ([WindowsUtils.SessionState]::Disconnected)
    }
}