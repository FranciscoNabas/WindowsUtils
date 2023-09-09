Describe 'Get-ResourceMessageTable' {
    It 'Gets a DLL message table' {
        $result = Get-ResourceMessageTable -Path '.\src\Pester\PesterMessageTest.dll'
        $result.Id | Should -Be 666
        $result.Message | Should -Be 'This is a very cool message that we not only will use for testing, but tits.'
    }
}