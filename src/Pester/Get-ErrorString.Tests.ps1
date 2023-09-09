Describe 'Get-ErrorString' {
    It 'Gets system error message strings' {
        Get-ErrorString -ErrorCode 5 | Should -Be 'Access is denied.'
    }

    It 'Gets NT error message strings' {
        Get-ErrorString 0xC0000005 NtError | Should -Be 'The instruction at 0x%p referenced memory at 0x%p. The memory could not be %s.'
    }

    It 'Gets FDI error message strings' {
        Get-ErrorString 6 FdiError | Should -Be 'Unknown compression type.'
    }
}