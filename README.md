# WindowsUtils
  
**WindowsUtils** is a PowerShell module designed to make easier the administration of Windows computers.  
The module consists in a library written in C++/CLI which contains the main functions, and the .NET wrapper, and an utilities-library written in C#.  
To get information on how to use it, use **Get-Help _Cmdlet-Name_ -Full**.  
  
- [WindowsUtils](#windowsutils)
  - [Installation](#installation)
  - [Cmdlets](#cmdlets)
    - [Invoke-RemoteMessage](#invoke-remotemessage)
    - [Get-RemoteMessageOptions](#get-remotemessageoptions)
    - [Get-ComputerSession](#get-computersession)
    - [Send-Click](#send-click)
    - [Get-ResourceMessageTable](#get-resourcemessagetable)
    - [Get-FormattedError](#get-formattederror)
    - [Get-LastWin32Error](#get-lastwin32error)
    - [Get-ObjectHandle](#get-objecthandle)
    - [Get-MsiProperties](#get-msiproperties)
    - [Disconnect-Session](#disconnect-session)
    - [Remove-Service](#remove-service)
    - [Get-ServiceSecurity](#get-servicesecurity)
    - [New-ServiceAccessRule](#new-serviceaccessrule)
    - [New-ServiceAuditRule](#new-serviceauditrule)
    - [Set-ServiceSecurity](#set-servicesecurity)
    - [Get-InstalledDotnet](#get-installeddotnet)
    - [Expand-Cabinet](#expand-cabinet)
    - [Start-Tcping](#start-tcping)
  - [Changelog](#changelog)
  - [Support](#support)
  
## Installation
  
This module is available on the [PowerShell Gallery](https://www.powershellgallery.com/packages/WindowsUtils).  
To install it, you can use **PowerShellGet**.  
  
```powershell
Install-Module -Name 'WindowsUtils'
Import-Module -Name 'WindowsUtils'
```
  
If you clone the repository, build **WindowsUtils.csproj**, and the module will be found at **.\bin\WindowsUtils**.  
To import it in a PowerShell session, use **Import-Module** on the module manifest or folder.  
  
```powershell
Import-Module -Name '.\bin\WindowsUtils'
Import-Module -Name '.\bin\WindowsUtils\WindowsUtils.psd1'
```
  
## Cmdlets
  
### Invoke-RemoteMessage
  
This Cmdlet is the **Windows Terminal Services** equivalent of the **MessageBox** function.  
It allows you to send messages on the local, or remote computers. On all or selected interactive sessions.  
  
```powershell
Invoke-RemoteMessage -Title 'Awesome message!' -Message 'Hello dude.' -SessionId 1 -Style 'MB_OKCANCEL','MB_ICONINFORMATION' -Timeout 30 -Wait
```
  
### Get-RemoteMessageOptions
  
This Cmdlet returns all options available to be used with the parameter **Style**, from **Invoke-RemoteMessage**.  
These options are named like the ones from the [MessageBox](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messagebox) function.  
  
```powershell
Get-RemoteMessageOptions | Format-Table * -Autosize
```
  
### Get-ComputerSession
  
This Cmdlet allows you to list sessions on the local and remote computers.  
  
```powershell
Get-ComputerSession -ComputerName 'MYQUANTUMCOMPUTER.contoso.com' -ActiveOnly -IncludeSystemSession
```
  
### Send-Click
  
Sends a click on the current desktop.  
  
```powershell
Send-Click
```
  
### Get-ResourceMessageTable
  
This Cmdlet was designed to retrieve messages stored in a message table, in a file.  
These files are usually libraries used by the system, and other applications, for error handling, status and many others.  
It is specially useful to build automations for **Microsof Endpoint Configuration Manager**, based on **Status Messages**.  
  
```powershell
Get-ResourceMessageTable -Path 'C:\Windows\System32\kernel32.dll'
```
  
### Get-FormattedError
  
This Cmdlet retrieves the message for a 'Win32' [System Error Code](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes).  
Trivia: These errors are stored in message tables, inside system DLLs, which you can list with the previous Cmdlet.  
  
```powershell
Get-FormattedError -ErrorCode 5

Access is denied.

[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-FormattedError
```
  
### Get-LastWin32Error
  
This Cmdlet returns the last 'Win32' error thrown by the system.  
Does the same as the **GetLastWin32Error()** method, from **System.Runtime.InteropServices.Marshal**, but brings the error message instead of the code.  
  
```powershell
Get-LastWin32Error
```  
  
### Get-ObjectHandle
  
My favorite one, and the one I had most fun building.  
This Cmdlet was designed to mimic the famous [Handle](https://learn.microsoft.com/en-us/sysinternals/downloads/handle), from Sysinternals.  
It shows which process holds a **handle** to a file or directory. This information can be used when you need to modify or delete a file locked by a process.  
You can close handles to objects using **-CloseHandle**.  
  
```powershell
Get-ObjectHandle -Path 'C:\Windows\System32\kernel32.dll', 'C:\Windows\System32\ntdll.dll'
Get-ObjectHandle -Path "$env:TEMP\*.tmp"

Get-ObjectHandle -Path "${env:ProgramFiles(x86)}\7-zip\7-zip.dll" -CloseHandle
Get-ObjectHandle -Path "${env:ProgramFiles(x86)}\7-zip\7-zip.dll" -CloseHandle -Force

PS C:\Windows\System32>_ Get-ObjectHandle csrss*
```
  
### Get-MsiProperties
  
Another favorite of mine, this Cmdlet get's information about MSI files, from the installer database.  
This can also be achieved using the [WindowsInstaller.Installer](https://learn.microsoft.com/en-us/windows/win32/msi/installer-object) object, and in fact, it uses the same object, but called directly via WINAPI.  
  
```powershell
Get-MsiProperties -Path 'C:\Users\You\Downloads\PowerShell-Installer.msi'
```
  
### Disconnect-Session
  
This Cmdlet disconnects interactive sessions on the local, or remote computers.  
It disconnects based on the **SessionId**, which can be obtained with **Get-ComputerSession**.  
  
```powershell
Disconnect-Session -ComputerName 'MYQUANTUMCOMPUTER.contoso.com' -SessionId 3 -Wait
```

### Remove-Service

This Cmdlet deletes a service installed in the current or remote computer.
You can choose to stop the service, and its dependents to make sure the service is deleted as soon as possible.
To remove a service is ultimately to mark it to deletion. The removal might not be instantaneous.

```powershell
# Removes the service 'MyCoolService'.
Remove-Service -Name 'MyCoolService'

# Stops the service, and its dependents, and remove it. 'Force' skips confirmation.
Remove-Service -Name 'MyCoolService' -Stop -Force
```

### Get-ServiceSecurity

This Cmdlet gets the security attributes for a service. These include the DACL, and optionally the SACL.
It was designed to be familiar with the '*-Acl' Cmdlet series, like `Get-Acl`. In fact, the objects where
created based on the same objects.

```powershell-console
Get-ServiceSecurity wuauserv | Format-List *

Name                    : wuauserv
AccessToString          : NT AUTHORITY\Authenticated Users Allow Start, GenericRead
                          NT AUTHORITY\SYSTEM Allow AllAccess
                          BUILTIN\Administrators Allow AllAccess
AuditToString           :
Access                  : {WindowsUtils.AccessControl.ServiceAccessRule, WindowsUtils.AccessControl.ServiceAccessRule, WindowsUtils.AccessControl.ServiceAccessRule}
Audit                   : {}
AccessRightType         : WindowsUtils.Services.ServiceRights
AccessRuleType          : WindowsUtils.AccessControl.ServiceAccessRule
AuditRuleType           : WindowsUtils.AccessControl.ServiceAuditRule
Owner                   : NT AUTHORITY\SYSTEM
Group                   : NT AUTHORITY\SYSTEM
Sddl                    : O:SYG:SYD:(A;;CCLCSWRPLORC;;;AU)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;SY)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)
AreAccessRulesProtected : False
AreAuditRulesProtected  : False
AreAccessRulesCanonical : True
AreAuditRulesCanonical  : True
```

```powershell-console
Get-ServiceSecurity wuauserv -Audit | Format-List *

Name                    : wuauserv
AccessToString          : NT AUTHORITY\Authenticated Users Allow Start, GenericRead
                          NT AUTHORITY\SYSTEM Allow AllAccess
                          BUILTIN\Administrators Allow AllAccess
AuditToString           : Everyone Failure ChangeConfig, Start, Stop, PauseContinue, Delete, GenericRead, WriteDac, WriteOwner
Access                  : {WindowsUtils.AccessControl.ServiceAccessRule, WindowsUtils.AccessControl.ServiceAccessRule, WindowsUtils.AccessControl.ServiceAccessRule}
Audit                   : {WindowsUtils.AccessControl.ServiceAuditRule}
AccessRightType         : WindowsUtils.Services.ServiceRights
AccessRuleType          : WindowsUtils.AccessControl.ServiceAccessRule
AuditRuleType           : WindowsUtils.AccessControl.ServiceAuditRule
Owner                   : NT AUTHORITY\SYSTEM
Group                   : NT AUTHORITY\SYSTEM
Sddl                    : O:SYG:SYD:(A;;CCLCSWRPLORC;;;AU)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;SY)(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)S:(AU;FA;CCDCLCSWRPWPDTLOSDRCWDWO;;;WD)
AreAccessRulesProtected : False
AreAuditRulesProtected  : False
AreAccessRulesCanonical : True
AreAuditRulesCanonical  : True
```

### New-ServiceAccessRule

This Cmdlet creates a service access rule (DACL) used to change service security.
You can use the new access rule with `Set-ServiceSecurity`.

```powershell-console
New-ServiceAccessRule -Identity 'NT AUTHORITY\SYSTEM' -Rights 'ChangeConfig' -Type 'Allow' -InheritanceFlags 'ObjectInherit' -PropagationFlags 'InheritOnly'

ServiceRights     : ChangeConfig
AccessControlType : Allow
IdentityReference : NT AUTHORITY\SYSTEM
IsInherited       : False
InheritanceFlags  : ObjectInherit
PropagationFlags  : InheritOnly
```

### New-ServiceAuditRule

This Cmdlet creates a service audit rule (SACL) used to change service security.
You can use the new audit rule with `Set-ServiceSecurity`.

```powershell-console
New-ServiceAuditRule -Identity 'NT AUTHORITY\SYSTEM' -Rights 'EnumerateDependents' -Flags 'Failure'

ServiceRights     : EnumerateDependents
AuditFlags        : Failure
IdentityReference : NT AUTHORITY\SYSTEM
IsInherited       : False
InheritanceFlags  : None
PropagationFlags  : None
```

### Set-ServiceSecurity

This Cmdlet changes service security. It was designed to work like the `Set-Acl` Cmdlet.
You typically use this Cmdlet with the other service security commands.

```powershell
$serviceSecurity = Get-ServiceSecurity -Name test_service
$newRule = New-ServiceAccessRule -Identity 'CONTOSO\User' -Rights 'EnumerateDependents' -Type 'Allow'
$serviceSecurity.AddAccessRule($newRule)
Set-ServiceSecurity -Name test_service -SecurityObject $serviceSecurity
```

### Get-InstalledDotnet

This Cmdlet returns all .NET versions installed in the computer. Additionally, you can return
the installed patches for .NET.

```powershell-console
Get-InstalledDotnet

Version        Edition ComputerName
-------        ------- ------------
4.8.9032 FullFramework MYCOMPUTERNAME
```

```powershell-console
Get-InstalledDotnet -InlcudeUpdate | Select-Object -ExpandProperty InstalledUpdates

Version                                   InstalledUpdates                                ComputerName
-------                                   ----------------                                ------------
Microsoft .NET Framework 4 Client Profile {KB2468871, KB2468871v2, KB2478063, KB2533523…} MYCOMPUTERNAME
Microsoft .NET Framework 4 Extended       {KB2468871, KB2468871v2, KB2478063, KB2533523…} MYCOMPUTERNAME
```

### Expand-Cabinet

This Cmdlet extracts files from a cabinet file.
This Cmdlet is provider-aware.

```powershell
# Extracts files from 'Cabinet.cab' to the 'Destination' folder.
Expand-Cabinet -Path "$env:SystemDrive\Path\To\Cabinet.cab" -Destination "$env:SystemDrive\Path\To\Destination"

# Extract files from all cabinet files from 'C:\CabinetSource' that matches 'MultipleCab*'.
Get-ChildItem -Path 'C:\CabinetSource\MultipleCab*' | Expand-Cabinet -Destination 'C:\Path\To\Destination'
```

### Start-Tcping

This Cmdlet attempts to measure network statistics while connecting to a destination using TCP.
It works similarly as well-known tools like 'ping.exe', or 'tcping.exe'.
The parameters contain aliases that mimic the parameter in those applications.

```powershell-console
Start-Tcping -Destination learn.microsoft.com  -Port 443 -IncludeJitter

Probing learn.microsoft.com [2600:1419:6200:284::3544] on port 443:
2600:1419:6200:284::3544 - TCP:443 - Port is open - time=10.84ms
2600:1419:6200:284::3544 - TCP:443 - Port is open - time=9.54ms jitter=1.31ms
2600:1419:6200:284::3544 - TCP:443 - Port is open - time=11.33ms jitter=1.14ms
2600:1419:6200:284::3544 - TCP:443 - Port is open - time=11.07ms jitter=0.51ms

Pinging statistics for 2600:1419:6200:284::3544:
        Packets: Sent = 4, Successful = 4, Failed = 0 (0%),
Approximate round trip times in milli-seconds:
        Minimum = 9.54ms, Maximum = 11.33ms, Average = 10.70ms,
Approximate jitter in milli-seconds:
        Minimum = 0.51ms, Maximum = 1.31ms, Average = 0.98m
```

```powershell-console
Start-Tcping learn.microsoft.com -j -d -t

Probing learn.microsoft.com [2600:1419:6200:289::3544] continuously on port 80 (press Ctrl + C to stop):
2023-8-29 23:44:52.751 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=6.43ms
2023-8-29 23:44:53.772 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=12.59ms jitter=6.16ms
2023-8-29 23:44:54.794 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=12.74ms jitter=3.23ms
2023-8-29 23:44:55.817 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=8.51ms jitter=2.07ms
2023-8-29 23:44:56.842 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=12.45ms jitter=2.38ms
2023-8-29 23:44:57.866 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=11.35ms jitter=0.80ms
2023-8-29 23:44:58.889 - 2600:1419:6200:289::3544 - TCP:80 - Port is open - time=8.79ms jitter=1.89ms

Pinging statistics for 2600:1419:6200:289::3544:
        Packets: Sent = 7, Successful = 7, Failed = 0 (0%),
Approximate round trip times in milli-seconds:
        Minimum = 8.51ms, Maximum = 12.74ms, Average = 10.41ms,
Approximate jitter in milli-seconds:
        Minimum = 0.80ms, Maximum = 6.16ms, Average = 2.76ms
```

## Changelog
  
Versioning information can be found on the [Changelog](https://github.com/FranciscoNabas/WindowsUtils/blob/main/CHANGELOG.md) file.  
Changelogging began at version 1.3.0, because I didn't keep track before that.  
  
## Support
  
If you have an idea, or a solution you'd like to have in PowerShell, Windows-related, regardless of how absurd it might sound, let me know. I'd love to try it.  
This is a module from a Sysadmin for Sysadmins, if you know how to program using C++/CLI and C#, and want to contribute, fork it!  
