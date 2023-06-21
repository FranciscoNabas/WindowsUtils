BeforeAll {
    # Importing the module.
    Set-Location -Path $PSScriptRoot
    Push-Location -Path '..\..\Release\WindowsUtils'
    Import-Module '.\WindowsUtils.psd1'
    Pop-Location

    #region Functions
    function New-WuLocalUser {
        $computer = [ADSI]"WinNT://$($env:COMPUTERNAME),computer"
        $user = $computer.Create("User", "Wu_Pma_User")
        $user.SetPassword("NotASafeP@ssword!")
        $user.Put("Description",'')    
        $user.SetInfo()    
    }
    function WuPester_Services_NewTestService {
        $new_service_splat = @{
            Name           = 'WindowsUtils_Pester_Service'
            StartupType    = 'Manual'
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

    # Creating the test user.
    if (!(Get-LocalUser -Name 'Wu_Pma_User' -ErrorAction SilentlyContinue)) {
        [void](New-WuLocalUser)
    }
    #endregion
}

Describe 'Set-ServiceSecurity by name' {
    It 'Changes access rules' {
        $serviceSecurity = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $previousState = $serviceSecurity

        $accessRule = New-Object -TypeName 'WindowsUtils.AccessControl.ServiceAccessRule' -ArgumentList @('Wu_Pma_User', 'AllAccess', 'Allow')
        $serviceSecurity.AddAccessRule($accessRule)
        $serviceSecurity.Access.Where({ $_ -eq $accessRule }) | Should -Not -BeNullOrEmpty

        # Setting by name.
        Set-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -SecurityObject $serviceSecurity
        
        # Testing.
        $newState = (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service').Access
        $newState.Count | Should -EQ $serviceSecurity.Access.Count
        foreach ($rule in $newState) {
            { $rule -in $serviceSecurity.Access } | Should -BeTrue
        }

        # Reseting by service controller.
        Set-ServiceSecurity -InputObject (Get-Service -Name 'WindowsUtils_Pester_Service') -SecurityObject $previousState

        # Testing.
        $newState = (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service').Access
        $newState.Count | Should -EQ $previousState.Access.Count
        foreach ($rule in $newState) {
            { $rule -in $previousState.Access } | Should -BeTrue
        }
    }
    It 'Changes the owner' {
        $serviceSecurity = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $newOwner = [System.Security.Principal.NTAccount]'Wu_Pma_User'
        $previousOwner = [System.Security.Principal.NTAccount]$serviceSecurity.Owner
        
        $serviceSecurity.SetOwner($newOwner)
        $serviceSecurity.Owner | Should -EQ $newOwner.ToString()

        # Setting by name.
        Set-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -SecurityObject $serviceSecurity

        # Testing.
        $newState = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $newState.Owner | Should -EQ $newOwner.ToString()

        # Reseting by service controller.
        $newState.SetOwner($previousOwner)
        $newState.Owner | Should -EQ $previousOwner.ToString()
        Set-ServiceSecurity -InputObject (Get-Service -Name 'WindowsUtils_Pester_Service') -SecurityObject $newState

        # Testing.
        $newState = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $newState.Owner | Should -EQ $previousOwner.ToString()
    }
    It 'Changes the owner' {
        $serviceSecurity = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $newGroup = [System.Security.Principal.NTAccount]'Administrators'
        $previousGroup = [System.Security.Principal.NTAccount]$serviceSecurity.Group
        
        $serviceSecurity.SetGroup($newGroup)
        $serviceSecurity.Group | Should -EQ $newGroup.ToString()

        # Setting by name.
        Set-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -SecurityObject $serviceSecurity

        # Testing.
        $newState = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $newState.Group | Should -EQ $newGroup.ToString()

        # Reseting by service controller.
        $newState.SetGroup($previousGroup)
        $newState.Group | Should -EQ $previousGroup.ToString()
        Set-ServiceSecurity -InputObject (Get-Service -Name 'WindowsUtils_Pester_Service') -SecurityObject $newState

        # Testing.
        $newState = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service'
        $newState.Group | Should -EQ $previousGroup.ToString()
    }
    It 'Changes audit rules' {
        $serviceSecurity = Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Audit
        $previousState = $serviceSecurity

        $auditRule = New-Object -TypeName 'WindowsUtils.AccessControl.ServiceAuditRule' -ArgumentList @('Wu_Pma_User', 'AllAccess', 'Failure')
        $serviceSecurity.AddAuditRule($auditRule)
        $serviceSecurity.Audit.Where({ $_ -eq $auditRule }) | Should -Not -BeNullOrEmpty
        
        # Setting by name.
        Set-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -SecurityObject $serviceSecurity -SetSacl
        
        # Testing.
        $newState = (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Audit).Audit
        $newState.Count | Should -EQ $serviceSecurity.Audit.Count
        foreach ($rule in $newState) {
            { $rule -in $serviceSecurity.Audit } | Should -BeTrue
        }

        # Reseting by service controller.
        Set-ServiceSecurity -InputObject (Get-Service -Name 'WindowsUtils_Pester_Service') -SecurityObject $previousState -SetSacl

        # Testing.
        $newState = (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service').Access
        $newState.Count | Should -EQ $previousState.Access.Count
        foreach ($rule in $newState) {
            { $rule -in $previousState.Access } | Should -BeTrue
        }
    }
    It 'Changes security with SDDL input' {
        $customSddl = 'O:S-1-5-21-1370324626-3833737555-3679571443-1001G:BAD:(A;;CCLCSWLOCRRC;;;IU)(A;;CCLCSWLOCRRC;;;SU)(A;;CCLCSWRPWPDTLOCRRC;;;SY)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;S-1-5-21-1370324626-3833737555-3679571443-1001)S:(AU;SA;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;S-1-5-21-1370324626-3833737555-3679571443-1001)'
        $previousSddl = (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Audit).Sddl
        
        Set-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Sddl $customSddl -SetSacl
        (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Audit).Sddl | Should -EQ $customSddl

        Set-ServiceSecurity -InputObject (Get-Service -Name 'WindowsUtils_Pester_Service') -Sddl $previousSddl -SetSacl
        (Get-ServiceSecurity -Name 'WindowsUtils_Pester_Service' -Audit).Sddl | Should -EQ $previousSddl
    }
}