#region Setup
$ErrorActionPreference = 'Stop'

function New-WuLocalUser {
    $computer = [ADSI]"WinNT://$($env:COMPUTERNAME),computer"
    $user = $computer.Create("User", "Wu_Pma_User")
    $user.SetPassword("NotASafeP@ssword!")
    $user.Put("Description",'')    
    $user.SetInfo()    
}

$succeededText = '    [+] - {0}: Passed.'
$failedText = '    [-] - {0}: Failed. {1}'
$passedCount = 0
$failedCount = 0
[System.Collections.Generic.List[System.Management.Automation.ErrorRecord]]$errorList

Write-Host 'Starting tests.' -ForegroundColor DarkMagenta
# Importing the module.
Set-Location -Path $PSScriptRoot
Push-Location -Path '..\..\Release\WindowsUtils'
Import-Module '.\WindowsUtils.psd1'
Pop-Location

# Creating temporary local user.
if (!(Get-LocalUser -Name 'Wu_Pma_User' -ErrorAction SilentlyContinue)) {
    [void](New-WuLocalUser)
}
#endregion

#region Testing with minimum input
try {
    $sacl = New-ServiceAuditRule -Identity 'Wu_Pma_User' -Rights 'AllAccess' -Flags 'Success'
    $fromConstructor = [WindowsUtils.AccessControl.ServiceAuditRule]::new(
        'Wu_Pma_User',
        [WindowsUtils.Engine.ServiceRights]::AllAccess,
        [System.Security.AccessControl.AuditFlags]::Success
    )
    if ($sacl -eq $fromConstructor) {
        Write-Host ($succeededText -f 'Minimum input') -ForegroundColor DarkGreen
        $passedCount++
    }
    else {
        Write-Host ($failedText -f 'Minimum input', 'Result not equal.') -ForegroundColor DarkRed
        $failedCount++
    }
}
catch {
    Write-Host ($failedText -f 'Minimum input', $_.Exception.Message) -ForegroundColor DarkRed
    [void]$errorList.Add($_)
    $failedCount++
}
#endregion

#region Testing with partial input
try {
    $fromConstructor = ([WindowsUtils.AccessControl.ServiceAuditRule]::new(
        'Wu_Pma_User',
        [WindowsUtils.Engine.ServiceRights]::EnumerateDependents -bor [WindowsUtils.Engine.ServiceRights]::QueryConfig,
        [System.Security.AccessControl.AuditFlags]::Failure
    ))
    $splat = @{
        Identity = 'Wu_Pma_User'
        Rights = ([WindowsUtils.Engine.ServiceRights]::EnumerateDependents -bor [WindowsUtils.Engine.ServiceRights]::QueryConfig)
        Flags = 'Failure'
    }

    $sacl = New-ServiceAuditRule @splat
    $splat.Rights = @([WindowsUtils.Engine.ServiceRights]::EnumerateDependents, [WindowsUtils.Engine.ServiceRights]::QueryConfig)
    $sacl2 = New-ServiceAuditRule @splat

    if ($sacl -eq $fromConstructor -and $sacl2 -eq $fromConstructor) {
        Write-Host ($succeededText -f 'Partial input') -ForegroundColor DarkGreen
        $passedCount++
    }
    else {
        Write-Host ($failedText -f 'Partial input', 'Result not equal.') -ForegroundColor DarkRed
        $failedCount++
    }
}
catch {
    Write-Host ($failedText -f 'Partial input', $_.Exception.Message) -ForegroundColor DarkRed
    [void]$errorList.Add($_)
    $failedCount++
}

#endregion

#region Testing with full input
try {
    $fromConstructor = [WindowsUtils.AccessControl.ServiceAuditRule]::new(
        'Wu_Pma_User',
        [WindowsUtils.Engine.ServiceRights]::EnumerateDependents -bor [WindowsUtils.Engine.ServiceRights]::QueryConfig,
        $true,
        [System.Security.AccessControl.InheritanceFlags]::ContainerInherit,
        [System.Security.AccessControl.PropagationFlags]::NoPropagateInherit,
        [System.Security.AccessControl.AuditFlags]::Success
    )
    $splat = @{
        Identity = 'Wu_Pma_User'
        Rights = @([WindowsUtils.Engine.ServiceRights]::EnumerateDependents, [WindowsUtils.Engine.ServiceRights]::QueryConfig)
        Inherited = $true
        InheritanceFlags = 'ContainerInherit'
        PropagationFlags = 'NoPropagateInherit'
        Flags = 'Success'
    }

    $sacl = New-ServiceAuditRule @splat
    if ($sacl -eq $fromConstructor) {
        Write-Host ($succeededText -f 'Full input') -ForegroundColor DarkGreen
        $passedCount++
    }
    else {
        Write-Host ($failedText -f 'Full input', 'Result not equal.') -ForegroundColor DarkRed
        $failedCount++
    }
}
catch {
    Write-Host ($failedText -f 'Full input', $_.Exception.Message) -ForegroundColor DarkRed
    [void]$errorList.Add($_)
    $failedCount++
}

#endregion

#region Cleanup
Remove-LocalUser -Name 'Wu_Pma_User' -Confirm:$false
#endregion

Write-Host "Finished. Total: $($passedCount + $failedCoun) " -ForegroundColor DarkMagenta -NoNewline
Write-Host "Passed: $passedCount " -ForegroundColor DarkGreen -NoNewline
Write-Host "Faled: $failedCount" -ForegroundColor DarkRed