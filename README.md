# WindowsUtils
  
**WindowsUtils** is a PowerShell module designed to make easier the administration of Windows computers.  
The module consists in a library written in C++/CLI which contains the main functions, and the .NET wrapper.  
To get information on how to use it, use **Get-Help _Cmdlet-Name_ -Full**.  
  
- [WindowsUtils](#windowsutils)
  - [Installation](#installation)
  - [Cmdlets](#cmdlets)
    - [Invoke-RemoteMessage](#invoke-remotemessage)
    - [Get-RemoteMessageOptions](#get-remotemessageoptions)
    - [Get-ComputerSession](#get-computersession)
    - [Send-Click](#send-click)
    - [Get-ResourceMessageTable](#get-resourcemessagetable)
    - [Get-ErrorString (gerrmess)](#get-errorstring-gerrmess)
    - [Get-LastWin32Error](#get-lastwin32error)
    - [Get-ObjectHandle (gethandle)](#get-objecthandle-gethandle)
    - [Get-MsiProperties](#get-msiproperties)
    - [Disconnect-Session (disconnect)](#disconnect-session-disconnect)
    - [Remove-Service](#remove-service)
    - [Get-ServiceSecurity](#get-servicesecurity)
    - [New-ServiceAccessRule](#new-serviceaccessrule)
    - [New-ServiceAuditRule](#new-serviceauditrule)
    - [Set-ServiceSecurity](#set-servicesecurity)
    - [Get-InstalledDotnet (getdotnet)](#get-installeddotnet-getdotnet)
    - [Expand-Cabinet](#expand-cabinet)
    - [Start-Tcping (tcping)](#start-tcping-tcping)
    - [Start-ProcessAsUser (runas)](#start-processasuser-runas)
    - [Get-NetworkFile (psfile, getnetfile)](#get-networkfile-psfile-getnetfile)
    - [Close-NetworkFile (closenetfile)](#close-networkfile-closenetfile)
    - [New-Cabinet](#new-cabinet)
    - [Test-Port (testport)](#test-port-testport)
    - [Get-ProcessModule (listdlls)](#get-processmodule-listdlls)
    - [Suspend-Process (suspend)](#suspend-process-suspend)
    - [Resume-Process (resume)](#resume-process-resume)
    - [Get-ErrorInformation (err)](#get-errorinformation-err)
    - [Get-MsiSummaryInfo](#get-msisummaryinfo)
    - [Get-MsiTableInfo](#get-msitableinfo)
    - [Get-MsiTableData](#get-msitabledata)
    - [Invoke-MsiQuery (imsisql)](#invoke-msiquery-imsisql)
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
  
### Get-ErrorString (gerrmess)
  
This Cmdlet retrieves the message for a 'Win32', 'NTSTATUS', 'FDI' and 'FCI' errors
Trivia: These errors are stored in message tables, inside system DLLs, which you can list with the previous Cmdlet.  
  
```powershell
Get-ErrorString -ErrorCode 5

Access is denied.

[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-FormattedError
```
  
### Get-LastWin32Error
  
This Cmdlet returns the last 'Win32' error thrown by the system.  
Does the same as the **GetLastWin32Error()** method, from **System.Runtime.InteropServices.Marshal**, but brings the error message instead of the code.  
  
```powershell
Get-LastWin32Error
```  
  
### Get-ObjectHandle (gethandle)
  
My favorite one, and the one I had most fun building.  
This Cmdlet was designed to mimic the famous [Handle](https://learn.microsoft.com/en-us/sysinternals/downloads/handle), from Sysinternals.  
It shows which process holds a **handle** to a file or directory. This information can be used when you need to modify or delete a file locked by a process.  
You can close handles to objects using **-CloseHandle**.  
  
```powershell
Get-ObjectHandle -Path 'C:\Windows\System32\kernel32.dll', 'C:\Windows\System32\ntdll.dll'
Get-ObjectHandle -Path "$env:TEMP\*.tmp"

Get-ObjectHandle -Path "${env:ProgramFiles(x86)}\7-zip\7-zip.dll" -CloseHandle
Get-ObjectHandle -Path "${env:ProgramFiles(x86)}\7-zip\7-zip.dll" -CloseHandle -Force

Get-ChildItem -Path 'C:\Windows\System32' -Filter '*.dll' | Get-ObjectHandle

gethandle -ProcessId 666
Get-Process -Id 666 | gethandle -All

PS C:\Windows\System32>_ Get-ObjectHandle csrss*
```
  
### Get-MsiProperties
  
Another favorite of mine, this Cmdlet get's information about MSI files, from the installer database.  
This can also be achieved using the [WindowsInstaller.Installer](https://learn.microsoft.com/en-us/windows/win32/msi/installer-object) object, and in fact, it uses the same object, but called directly via WINAPI.  
  
```powershell
Get-MsiProperties -Path 'C:\Users\You\Downloads\PowerShell-Installer.msi'
```
  
### Disconnect-Session (disconnect)
  
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

### Get-InstalledDotnet (getdotnet)

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

### Start-Tcping (tcping)

This Cmdlet attempts to measure network statistics while connecting to a destination using TCP.
It works similarly as well-known tools like 'ping.exe', or 'tcping.exe'.
The parameters contain aliases that mimic the parameter in those applications.

```powershell-console
Start-Tcping -Destination learn.microsoft.com  -Port 443 -IncludeJitter

Destination                    Port  Status  Times
-----------                    ----  ------  -----
learn.microsoft.com            443   Open    Rtt: 7.14
learn.microsoft.com            443   Open    Rtt: 8.50, Jitter: 1.36
learn.microsoft.com            443   Open    Rtt: 9.50, Jitter: 1.68
learn.microsoft.com            443   Open    Rtt: 7.59, Jitter: 0.79

Sent          : 4
Succeeded     : 4
Failed        : 0
FailedPercent : 0.00%
MinTimes      : Rtt: 7.59, Jitter: 0.79
AvgTimes      : Rtt: 8.18, Jitter: 1.28
MaxTimes      : Rtt: 9.50, Jitter: 1.68
```

```powershell-console
tcping learn.microsoft.com, google.com -Port 80, 443 -s

Destination                    Port  Status  Times
-----------                    ----  ------  -----
learn.microsoft.com            80    Open    Rtt: 8.32
learn.microsoft.com            443   Open    Rtt: 19.91
google.com                     80    Open    Rtt: 9.57
google.com                     443   Open    Rtt: 7.01
```

### Start-ProcessAsUser (runas)

This Cmdlet logs in a user and starts a process with it.
This is my terrible attempt to reverse engineer 'runas.exe'.
It works by typing the credentials in the go, like 'runas.exe', or using
a `PSCredential`.

```powershell
Start-ProcessAsUser CONTOSO\francisco.nabas powershell
```

Of course we have an alias for it.

```powershell
runas -CommandLine powershell -Credential (Get-Credential)
```

### Get-NetworkFile (psfile, getnetfile)

This Cmdlet lists all files opened through the network on the current or remote computer.
It was designed to mimic `psfile.exe` from Sysinternals.

```powershell-console
Get-NetworkFile -ComputerName CISCOSRVP01P

Id Path                                                      LockCount UserName        Permissions
-- ----                                                      --------- --------        -----------
8  C:\                                                       0         francisco.nabas Read
14 C:\Program Files                                          0         francisco.nabas Read
18 C:\Program Files\Microsoft Analysis Services              0         francisco.nabas Read
24 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
25 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
28 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
29 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
40 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
43 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
56 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
59 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
60 \srvsvc                                                   0         francisco.nabas Read, Write, ChangeAttribute
```

```powershell-console
psfile CISCOSRVP01P -IncludeConnectionName | Select-object * -First 1

ComputerName : CISCOSRVP01P
SessionName  : 10.21.13.152
UserName     : francisco.nabas
LockCount    : 0
Permissions  : Read
Path         : C:\
Id           : 8
```

```powershell-console
psfile CISCOSRVP01P -BasePath 'C:\Program Files\Microsoft Analysis Services'

Id Path                                                      LockCount UserName        Permissions
-- ----                                                      --------- --------        -----------
18 C:\Program Files\Microsoft Analysis Services              0         francisco.nabas Read
24 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
25 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
28 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
29 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
40 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
43 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
59 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
```

```powershell-console
getnetfile CISCOSRVP01P -UserConnectionFilter 'francisco.nabas'

Id Path                                                      LockCount UserName        Permissions
-- ----                                                      --------- --------        -----------
8  C:\                                                       0         francisco.nabas Read
14 C:\Program Files                                          0         francisco.nabas Read
18 C:\Program Files\Microsoft Analysis Services              0         francisco.nabas Read
24 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
25 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
28 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
29 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
40 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
43 C:\Program Files\Microsoft Analysis Services\AS OLEDB     0         francisco.nabas Read
59 C:\Program Files\Microsoft Analysis Services\AS OLEDB\140 0         francisco.nabas Read
64 \srvsvc                                                   0         francisco.nabas Read, Write, ChangeAttribute
```

### Close-NetworkFile (closenetfile)

This Cmdlet closes files opened through the network on the local or remote computer.
It was designed to work in conjunction with `Get-NetworkFile`.
It is also based on `psfile.exe`, from Sysinternals.

```powershell-console
Get-NetworkFile -ComputerName CISCOSRVP01P

Id  Path                                                                                                                    LockCount UserName        Permissions
--  ----                                                                                                                    --------- --------        -----------
24  C:\Program Files\Microsoft Analysis Services\AS OLEDB                                                                   0         francisco.nabas Read
25  C:\Program Files\Microsoft Analysis Services\AS OLEDB                                                                   0         francisco.nabas Read
28  C:\Program Files\Microsoft Analysis Services\AS OLEDB\140                                                               0         francisco.nabas Read
29  C:\Program Files\Microsoft Analysis Services\AS OLEDB\140                                                               0         francisco.nabas Read
59  C:\Program Files\Microsoft Analysis Services\AS OLEDB\140                                                               0         francisco.nabas Read
140 C:\Windows\SYSVOL\sysvol\nabas.com\Policies\{31B2F340-016D-11D2-945F-00C04FB984F9}\Machine                              0         CISCOCS01P$     Read
141 C:\Windows\SYSVOL\sysvol\nabas.com\Policies\{31B2F340-016D-11D2-945F-00C04FB984F9}\Machine\Microsoft                    0         CISCOCS01P$     Read
142 C:\Windows\SYSVOL\sysvol\nabas.com\Policies\{31B2F340-016D-11D2-945F-00C04FB984F9}\Machine\Microsoft\Windows NT         0         CISCOCS01P$     Read
143 C:\Windows\SYSVOL\sysvol\nabas.com\Policies\{31B2F340-016D-11D2-945F-00C04FB984F9}\Machine\Microsoft\Windows NT\SecEdit 0         CISCOCS01P$     Read
147 \srvsvc

Close-NetworkFile -ComputerName CISCOSRVP01P -FileId 24
```

```powershell
getnetfile CISCOSRVP01P | ? UserName -eq 'francisco.nabas' | closenetfile -Force
```

### New-Cabinet

This Cmdlet creates a new cabinet based on a source path. It must be a valid path to a file or folder.
If the path is a folder, it will search for files recursively, and compress them.
Cabinet files only accept files up to 2Gb of length, and the maximum size for a cabinet is also 2Gb.

```powershell
New-Cabinet -Path 'C:\Path\To\Files' -Destination 'C:\Path\To\Destination'
```

You can also limit the size of the cabinet, in kilobytes. If the files surpass this limit they will span over multiple files

```powershell
New-Cabinet -Path 'C:\Path\To\Files' -Destination 'C:\Path\To\Destination' -MaxCabSize 20000
```

### Test-Port (testport)

This Cmdlet tests if a TCP or UDP port is open in a given destination.
Attention! Due to the nature of UDP packets, `Test-Port` might return false positive if a timeout occur when testing against public domains, like 'google.com'.
UPD testing is better used with LAN servers.

```powershell
Test-Port -ComputerName 'google.com' -TcpPort 80
```

```powershell
testport 'SUPERSERVER.contoso.com' 443
```

```powershell
testport 'SUPERSERVER1.contoso.com', 'SUPERSERVER2.contoso.com' -TcpPort 80, 443, 1433 -UdpPort 67, 68, 69, 4011
```

### Get-ProcessModule (listdlls)

This Cmdlet lists modules loaded into processes. You can list modules for one or more processes, or all of them.
You can also include module file version information (with a performance penalty).

```powershell
Get-ProcessModule -Name 'explorer'
```

```powershell
listdlls -ProcessId 666, 667 -IncludeVersionInfo
```

```powershell
listdlls
```

### Suspend-Process (suspend)

This Cmdlet suspends a running process.

```powershell
Suspend-Process -Name 'notepad'
```

```powershell
suspend -Id 666
```

### Resume-Process (resume)

This Cmdlet resumes a suspended process. It does nothing on running processes.

```powershell
Resume-Process -Name 'notepad'
```

```powershell
resume -Id 666
```

### Get-ErrorInformation (err)

This Cmdlet mimics 'Err.exe'. In fact, the error database is extracted from it.

```powershell
Get-ErrorInformation -ErrorCode 0xC0000008
```

```powershell-console
err 0x80070057

ErrorCode   HexCode    IsHResult SymbolicName               Description
---------   -------    --------- ------------               -----------
-2147024809 0x80070057 True      XNS_INTERNAL_ERROR
-2147024809 0x80070057 False     COR_E_ARGUMENT             An argument does not meet the contract of the method.
-2147024809 0x80070057 False     DDERR_INVALIDPARAMS
-2147024809 0x80070057 False     DIERR_INVALIDPARAM
-2147024809 0x80070057 False     DSERR_INVALIDPARAM
-2147024809 0x80070057 True      NMERR_FRAME_HAS_NO_CAPTURE
-2147024809 0x80070057 False     STIERR_INVALID_PARAM
-2147024809 0x80070057 False     DRM_E_INVALIDARG
-2147024809 0x80070057 True      ERROR_INVALID_PARAMETER    The parameter is incorrect.
-2147024809 0x80070057 False     E_INVALIDARG               One or more arguments are invalid
-2147024809 0x80070057 True      LDAP_FILTER_ERROR
```

### Get-MsiSummaryInfo

This Cmdlet gets the summary information from a Windows Installer.
Summary data contains information about the author, languages, platforms, creation time, UAC compliance and more.

```powershell
Get-MsiSummaryInfo -Path 'C:\Path\To\Installer.msi'
```

### Get-MsiTableInfo

This Cmdlet gets table information from a Windows Installer's database.
You can list information about a single table, multiple tables, or all tables in the database. Contains the table name and column information,
like type, position, if it's key, nullable, etc.

```powershell
Get-MsiTableInfo -Path 'C:\Path\To\Installer.msi' -Table 'Feature', 'Registry'
```

```powershell
Get-MsiTableInfo 'C:\Path\To\Installer.msi'
```

### Get-MsiTableData

This Cmdlet gets the data from a table in a Windows Installer's Database.
This works for built-in, and custom tables.

```powershell
Get-MsiTableData -Path 'C:\Path\To\Installer.msi' -Table 'Feature', 'Registry'
```

```powershell
Get-MsiTableData 'C:\Path\To\Installer.msi' 'Feature'
```

```powershell
Get-MsiTableInfo -Path 'C:\Path\To\Installer.msi' -Table 'Feature', 'Registry' | Get-MsiTableData
```

### Invoke-MsiQuery (imsisql)

This Cmdlet executes a query in a Windows Installer MSI database.
ATTENTION! The SQL language used in the installer database is quite finicky. Be sure to check the notes, and related links for more information.
You can reach that information with `Get-Help Invoke-MsiQuery -Full`.

```powershell
Invoke-MsiQuery -Path 'C:\SuperInstaller.msi' -Query 'SELECT * FROM Registry' | Format-Table -AutoSize
```

```powershell
Invoke-MsiQuery 'C:\SuperInstaller.msi' "INSERT INTO Registry (Registry, Root, ``Key``, Name, Value, Component_) VALUES ('NeatNewKey', 0, 'SOFTWARE\NeatKey', 'NeatKey', 'KeyValue', 'KeyComponent')"
```

```powershell
$parameter = [WindowsUtils.Installer.InstallerCommandParameter]::new('String', 'ProductCode')
imsisql 'C:\SuperInstaller.msi' 'Select * From Property Where Property = ?' -Parameters $parameter
```

```powershell
$parameter = [WindowsUtils.Installer.InstallerCommandParameter]::new('File', 'C:\Path\To\InstallerIcon.ico')
imsisql 'C:\SuperInstaller.msi' "INSERT INTO Icon (Name, Data) VALUES ('IconName', ?)" -Parameters $parameter
```

## Changelog
  
Versioning information can be found on the [Changelog](https://github.com/FranciscoNabas/WindowsUtils/blob/main/CHANGELOG.md) file.  
Changelogging began at version 1.3.0, because I didn't keep track before that.  
  
## Support
  
No way you made it down here LOL.  
If you have an idea, or a solution you'd like to have in PowerShell, Windows-related, regardless of how absurd it might sound, let me know. I'd love to try it.  
This is a module from a Sysadmin to Sysadmins, if you know how to program using C++/CLI and C#, and want to contribute, fork it!  
