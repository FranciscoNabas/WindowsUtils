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

## Changelog
  
Versioning information can be found on the [Changelog](https://github.com/FranciscoNabas/WindowsUtils/blob/main/CHANGELOG.md) file.  
Changelogging began at version 1.3.0, because I didn't keep track before that.  
  
## Support
  
If you have an idea, or a solution you'd like to have in PowerShell, Windows-related, regardless of how absurd it might sound, let me know. I'd love to try it.  
This is a module from a Sysadmin for Sysadmins, if you know how to program using C++/CLI and C#, and want to contribute, fork it!  