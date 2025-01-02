BeforeAll {
    $Global:temp_path = [System.IO.Path]::GetTempFileName()
    Remove-Item -Path $Global:temp_path -Force -ErrorAction SilentlyContinue
    $Global:file_stream = [System.IO.File]::Open($temp_path, 'Create', 'ReadWrite', 'None')
    Remove-Item -Path 'HKLM:\SOFTWARE\WuPesterTestEphemeralKey' -Force -ErrorAction SilentlyContinue
    $Global:registryKey = [Microsoft.Win32.Registry]::LocalMachine.CreateSubKey('SOFTWARE\WuPesterTestEphemeralKey')
}

Describe 'Get-ObjectHandle' {
    It 'Gets open handles to a file' {
        $result = Get-ObjectHandle -Path $Global:temp_path
        $result.Type | Should -Be ([WindowsUtils.ObjectHandleType]::FileSystem)
        $result.InputObject | Should -Be ([System.IO.Path]::GetFileName($Global:temp_path))
        $result.ProcessId | Should -Be $PID
    }

    It 'Gets open handles to a registry key' {
        $result = gethandle -Path 'HKLM:\SOFTWARE\WuPesterTestEphemeralKey'
        $result.Type | Should -Be ([WindowsUtils.ObjectHandleType]::Registry)
        $result.InputObject | Should -Be 'WuPesterTestEphemeralKey'
        $result.ProcessId | Should -Be $PID
    }

    It 'Closes a file handle' {
        [void](Get-ObjectHandle -Path $Global:temp_path -CloseHandle -Force)
        Get-ObjectHandle -Path $Global:temp_path | Should -BeNullOrEmpty
    }

    It 'Closes a registry key handle' {
        [void](gethandle -Path 'HKLM:\SOFTWARE\WuPesterTestEphemeralKey' -CloseHandle -Force)
        gethandle -Path 'HKLM:\SOFTWARE\WuPesterTestEphemeralKey' | Should -BeNullOrEmpty
    }
}

AfterAll {
    Remove-Item -Path $Global:temp_path -Force -ErrorAction SilentlyContinue
    [Microsoft.Win32.Registry]::LocalMachine.DeleteSubKey('SOFTWARE\WuPesterTestEphemeralKey')
}