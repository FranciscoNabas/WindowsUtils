BeforeAll {
    $WarningPreference = 'SilentlyContinue'
    
    function WuCreateAndStartTestService {
        $newSvcSplat = @{
            Name = 'WuPesterTestService'
            BinaryPathName = "C:\LocalRepositories\.WindowsUtils\src\Pester\PesterServiceTest.exe"
            StartupType = 'Manual'
            Description = 'The most amazing service!'
        }
        
        if (Get-Service -Name 'WuPesterTestService' -ErrorAction SilentlyContinue) {
            [void](. sc.exe DELETE 'WuPesterTestService')
        }

        [void](New-Service @newSvcSplat)
        Start-Service -Name 'WuPesterTestService'
    }

    WuCreateAndStartTestService
}

Describe 'Remove-Service' {
    It 'Removes a service by name, with stop' {
        Remove-Service -Name 'WuPesterTestService' -Stop
        Get-Service -Name 'WuPesterTestService' -ErrorAction SilentlyContinue | Should -BeNullOrEmpty
    }

    It 'Removes a service by name, with stop and no wait' {
        WuCreateAndStartTestService

        $stopWatch = [System.Diagnostics.Stopwatch]::StartNew()
        Remove-Service -Name 'WuPesterTestService' -Stop -NoWait
        Start-Sleep -Seconds 6
        $stopWatch.Stop()

        # The service takes 5 seconds to stop. The elapsed needs to be between 6 and 10.
        if ($stopWatch.Elapsed.TotalSeconds -ge 10) {
            throw "-NoWait didn't worked."
        }

        Get-Service -Name 'WuPesterTestService' -ErrorAction SilentlyContinue | Should -BeNullOrEmpty
    }

    It 'Removes a service with a service controller' {
        WuCreateAndStartTestService

        # Running on a separate scope to make sure the GC discards any references to the service controller.
        $service = Get-Service -Name 'WuPesterTestService'
        $service | Remove-Service -Stop

        Start-Sleep -Seconds 2
        Get-Service -Name 'WuPesterTestService' -ErrorAction SilentlyContinue | Should -BeNullOrEmpty
    }

    It 'Removes a service by name, with no stop' {
        WuCreateAndStartTestService

        Remove-Service -Name 'WuPesterTestService'
        (Get-Service -name 'WuPesterTestService').Status | Should -Be 'Running'

        # Stop-Service throws an exception if the service is deleted at the end.
        # -ea SilentlyContinue doesn't stop it.
        try {
            Stop-Service -Name 'WuPesterTestService' -Force -ErrorAction Stop
        }
        catch { }

        Get-Service -Name 'WuPesterTestService' -ErrorAction SilentlyContinue | Should -BeNullOrEmpty
    }
}