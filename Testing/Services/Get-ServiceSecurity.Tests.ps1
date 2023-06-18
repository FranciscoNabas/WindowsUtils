BeforeAll {
    # Importing the module.
    Set-Location -Path $PSScriptRoot
    Push-Location -Path '..\..\Release\WindowsUtils'
    Import-Module '.\WindowsUtils.psd1'
    Pop-Location
}

Describe "Get-ServiceSecurity" {
    BeforeAll {
        #region Functions
        function WuPester_Services_NewTestService {
            $new_service_splat = @{
                Name = 'WindowsUtils_Pester_Service'
                StartupType = 'Manual'
                BinaryPathName = (Get-ChildItem -Path '..\WindowsUtilsTestService\bin\Release\WindowsUtilsTestService.exe').FullName
            }
            $managed_service = New-Service @new_service_splat
            try {
                Start-Service -InputObject $managed_service -ErrorAction 'Stop'
            }
            catch {
                throw "Failed to start service 'WindowsUtilsTestService'. $($_.Exception.Message) Cannot proceed."
            }
    
            $set_result = & sc.exe SDSET $managed_service.Name 'O:SYG:SYD:(A;;CCLCSWRPWPDTLOCRRC;;;SY)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)(A;;CCLCSWLOCRRC;;;IU)(A;;CCLCSWLOCRRC;;;SU)S:(AU;FA;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;WD)'
            if ($set_result -ne '[SC] SetServiceObjectSecurity SUCCESS') {
                throw "Call to 'sc.exe sdset' returned '$set_result'. Cannot continue."
            }
        }
        function WuPester_Services_AuthorizationRuleToString($acl) {
            if (!$acl) {
                return ''
            }
            $first = $true
            $output = ''
            foreach ($ace in $acl) {
                if ($first) { $first = $false }
                else { $output += "`n" }
    
                $output += "$($ace.IdentityReference) "
                $output += "$($ace.AccessControlType) "
                $output += $ace.ServiceRights.ToString()
            }
    
            return $output
        }
        function WuPester_Services_CompareSddlAces {
            
            param($Original, $ToCompare)

            $Original = ($Original | Select-String -Pattern '(?<=D:).*$').Matches[0].Value
            $ToCompare = ($ToCompare | Select-String -Pattern '(?<=D:).*$').Matches[0].Value
            $original_aces = $Original.Split(')').Where({ ![string]::IsNullOrEmpty($_) })
            
            foreach ($ace in $ToCompare.Split(')').Where({ ![string]::IsNullOrEmpty($_) })) {
                if ($ace -notin $original_aces) {
                    return $false
                }
            }

            return $true
        }
        #endregion
        
        # Creating the test service.
        if (Get-Service -Name 'WindowsUtils_Pester_Service' -ErrorAction 'SilentlyContinue') {
            # The chicken and the egg thingmabob.
            $remove_result = & sc.exe DELETE WindowsUtils_Pester_Service
            if ($remove_result -ne '[SC] DeleteService SUCCESS') {
                throw "Call to the 'sc.exe DELETE WindowsUtils_Pester_Service' command returned '$remove_result'. Delete the service before proceeding."
            }
        }
        WuPester_Services_NewTestService
        #endregion
    }
    
    It "Retrieves the 'ServiceSecurity' object for a service" {
        $service_security = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $service_security | Should -BeOfType ([WindowsUtils.AccessControl.ServiceSecurity])
    }
    It "Throws a 'NativeException' if the service does not exist" {
        $shouldSplat = @{
            'Throw' = [switch]::Present
            ExceptionType = ([WindowsUtils.NativeException])
            ExpectedMessage = "The specified service does not exist as an installed service.`r`n"
        }
        { Get-ServiceSecurity -Name 'ThisServiceDefinitely_DoesNotExist69' } | Should @shouldSplat 
    }
    It "Always returns a valid 'SDDL', 'Access', 'AccessToString, 'Name' and 'Owner'." {
        # Testing Name.
        $service_security = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $service_security.Name | Should -EQ 'WindowsUtils_Pester_Service'

        # Testing Sddl.
        $is_same_dacl = WuPester_Services_CompareSddlAces -Original $service_security.Sddl -ToCompare 'O:SYG:SYD:(A;;CCLCSWRPWPDTLOCRRC;;;SY)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)(A;;CCLCSWLOCRRC;;;IU)(A;;CCLCSWLOCRRC;;;SU)'
        $is_same_dacl | Should -BeTrue
        $raw_sec_desc = [System.Security.AccessControl.RawSecurityDescriptor]::new($service_security.Sddl)
        $raw_sec_desc | Should -BeOfType ([System.Security.AccessControl.RawSecurityDescriptor])
        
        # Testing Owner.
        $service_security.Owner | Should -Not -BeNullOrEmpty
        $service_security.Owner | Should -EQ $raw_sec_desc.Owner.Translate([System.Security.Principal.NTAccount]).ToString()

        # Testing Access.
        $service_security.Access | Should -BeOfType ([WindowsUtils.AccessControl.ServiceAccessRule[]])
        $service_security.Access.Count | Should -ExpectedValue 4
        foreach ($access_rule in $service_security.Access) {
            switch ($access_rule.IdentityReference.ToString()) {
                'NT AUTHORITY\INTERACTIVE' {
                    $newRuleArgs = @(
                        'NT AUTHORITY\INTERACTIVE'
                        [WindowsUtils.Engine.ServiceRights]::UserDefinedControl -bor [WindowsUtils.Engine.ServiceRights]::GenericRead
                        'Allow'
                    )
                    $access_rule | Should -EQ (New-Object -TypeName WindowsUtils.AccessControl.ServiceAccessRule -ArgumentList $newRuleArgs)
                }
                'NT AUTHORITY\SERVICE' {
                    $newRuleArgs = @(
                        'NT AUTHORITY\SERVICE'
                        [WindowsUtils.Engine.ServiceRights]::UserDefinedControl -bor [WindowsUtils.Engine.ServiceRights]::GenericRead
                        'Allow'
                    )
                    $access_rule | Should -EQ (New-Object -TypeName WindowsUtils.AccessControl.ServiceAccessRule -ArgumentList $newRuleArgs)
                }
                'NT AUTHORITY\SYSTEM' {
                    $newRuleArgs = @(
                        'NT AUTHORITY\SYSTEM'
                        [WindowsUtils.Engine.ServiceRights]::QueryConfig -bor `
                        [WindowsUtils.Engine.ServiceRights]::QueryStatus -bor `
                        [WindowsUtils.Engine.ServiceRights]::EnumerateDependents -bor `
                        [WindowsUtils.Engine.ServiceRights]::Interrogate -bor `
                        [WindowsUtils.Engine.ServiceRights]::GenericExecute
                        'Allow'
                    )
                    $access_rule | Should -EQ (New-Object -TypeName WindowsUtils.AccessControl.ServiceAccessRule -ArgumentList $newRuleArgs)
                }
                'BUILTIN\Administrators' {
                    $newRuleArgs = @(
                        'BUILTIN\Administrators'
                        [WindowsUtils.Engine.ServiceRights]::AllAccess
                        'Allow'
                    )
                    $access_rule | Should -EQ (New-Object -TypeName WindowsUtils.AccessControl.ServiceAccessRule -ArgumentList $newRuleArgs)
                }
                Default {
                    throw "Unexpected identity '$($access_rule.IdentityReference.ToString())' in service access rule list."
                }
            }
        }

        # Testing AccessToString.
        $service_security.AccessToString | Should -EQ (WuPester_Services_AuthorizationRuleToString($service_security.Access))
    }

    It "Returns a valid 'SystemAuditRule' array when called with '-Audit'." {
        $service_security = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Audit
        $service_security.Audit | Should -BeOfType ([WindowsUtils.AccessControl.ServiceAuditRule])
        $service_security.Audit.Count | Should -EQ 1
        $service_security.Audit[0] | Should -EQ ([WindowsUtils.AccessControl.ServiceAuditRule]::new('Everyone', 'AllAccess', 'Failure'))
    }

    AfterAll {
        Stop-Service -Name 'WindowsUtils_Pester_Service' -Force -ErrorAction 'SilentlyContinue'
        
        $delete_result = & sc.exe DELETE 'WindowsUtils_Pester_Service'
        if ($delete_result -ne '[SC] DeleteService SUCCESS') {
            if (!($delete_result -match 'FAILED 1060:')) {
                Write-Warning "Failed deleting test service. 'sc.exe DELETE' returned '$delete_result'. Remove the service manually before running the test again."
            }
        }
    }
}