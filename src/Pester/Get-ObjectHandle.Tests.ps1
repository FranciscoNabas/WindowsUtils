BeforeAll {
    Import-Module .\Release\WindowsUtils\WindowsUtils.psd1
    $Global:temp_path = [System.IO.Path]::GetTempFileName()
    $Global:file_stream = [System.IO.File]::Open($temp_path, 'OpenOrCreate', 'ReadWrite', 'None')
}

Describe 'Get-ObjectHandle' {
    It 'Gets open handles to a file' {
        $result = Get-ObjectHandle -Path $Global:temp_path
        $result.Type | Should -Be ([WindowsUtils.ObjectHandleType]::FileSystem)
        $result.InputObject | Should -Be ([System.IO.Path]::GetFileName($Global:temp_path))
        $result.ProcessId | Should -Be $PID
    }
}

AfterAll {
    $Global:file_stream.Close()
    Remove-Item -Path $Global:temp_path -Force -ErrorAction SilentlyContinue
}