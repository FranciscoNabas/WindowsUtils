# Changelog
  
All notable changes to this project will be documented in this file.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/), from version **1.3.0** on.  
  
## [1.4.0] - 2022-10-12
  
### Bugs
  
-   Some module functionalities depend on VC++ libraries. To avoid having to install the redistributable runtime, these assemblies are provided with the module.  
-   Invoke-RemoteMessage:  
    -   Sessions who doesn't support message box was returning the same response of the last session.  
    -   When 'Wait' was called with 'Timeout' equals to zero, and there are more than one valid session on the target computer, the console would wait all session's response.  
        This causes the console to hang if there were missing responses.  
        Now, if Wait is specified, Timeout is mandatory, and an error is thrown if the user inputs zero.  

### Added Features
  
-   StageComputerSession() method added to WtsSession to manage persistent WTS computer sessions.  
-   Get-ObjectHandle new alias 'gethandle'.  
-   New output object for Invoke-RemoteMessage. MessageResponse, with Session ID and Response.  
  
### Changed Features
  
-   Release notes on the module manifest, thus also on PowerShell Gallery now points to this document.  
-   WtsSession as a static class to use it as a 'Global Object'.  
-   Send-Click now is called from unmanaged code using SendInput. Previously was called using explicit PInvoke.  
-   Disconnect-Session parameter 'SessionId' not mandatory when disconnecting a session on the local computer. It will log off the current session.  
-   All output objects are now .NET wrapper classes for the CLR ref class. This helps with cyclical dependencies, and further module expansion.  
-   Get-ObjectHandle parameter 'Path' not mandatory anymore. It will query objects in the current directory.  
-   Invoke-RemoteMessage:  
    -   Now supports should process. Previously confirmation was managed by the UtilitiesLibrary.  
    -   Converting SessionId and Response vectors to smart pointer. Previously they were statically allocated/Deallocated on the heap.  
    -   Unmanaged function now returns a DWORD and receives arguments as reference. This is part of the unmanaged code, and error handling standardization.  
    -   Ignoring response type 0. This is returned when the session does not support message boxes.  
  
### Removed Features
  
-   UtilitiesLibrary was completely removed from the project. All functionalities were migrated to the module itself. The 'Wrapper' library now is called 'Core', contained in its own namespace.  
    This solves cyclical dependency issues, and helps standardize the output objecs.  
  
## [1.3.4] - 2022-10-08

Version 1.3.4 improves interoperability between existing WTS Cmdlets.  
  
## Changed Features
  
-   Get-ComputerSession had an additional property on its output object. ComputerName returns a value when the cmdlet is run for a remote computer.  
    This allows the output to be passed to Disconnect-Session.  
-   Disconnect-Session parameters ComputerName and SessionId now accepts pipeline input by property name.  
  
## [1.3.3] - 2022-10-07
  
### Changed Features
  
-   Get-FileHandle changed to Get-ObjectHandle. This Cmdlet also works with directories, and allow future expansion to other system objects.  
-   Get-ObjectHandle 'FileName' property changed to 'InputObject' to comply with the Cmdlet scope.  
  
### Bugs
  
-   Get-ObjectHandle was not returning a considerable number of image properties. This was due the Cmdlet using a Shell interface to get properties from the files themselves.  
    This implementation was replaced by [VerQueryValue](https://learn.microsoft.com/en-us/windows/win32/api/winver/nf-winver-verqueryvaluew).  
    Besides increasing performance, the only cases where properties are not shown is when the image does not contain a resource section, or access denied to the process.  
  
## [1.3.2] - 2022-10-05
  
Version 1.3.2 fixes bugs with Get-FileHandle Cmdlet
  
### Added Features
  
-   Get-FileHandle accepts wildcard.  
  
### Bugs
  
-   Get-FileHandle crashed when receiving pipeline input from Get-ChildItem, only on Windows PowerShell.  
    Cmdlet was adapted to be "Provider-Aware". Recommendations found on [this](https://stackoverflow.com/questions/8505294/how-do-i-deal-with-paths-when-writing-a-powershell-cmdlet) question.  

## [1.3.1] - 2022-10-04
  
Version 1.3.1 fixes bugs with the PS help engine.
  
### Added Features
  
-   Using XmlDoc2CmdletDoc for creating and managing help files.  

### Bugs
  
-   Help result not showing proper parameter information, and examples in the wrong category.
  
## [1.3.0] - 2022-10-02
  
Version 1.3.0 fixes major bugs and changes features.  
  
### Added Features
  
-   This project now have a Readme, and a Changelog!  
-   Adhering to Semantic Versioning.

### Changed Features
  
-   Get-FileHandle engine migrated from using the [Restart Manager](https://learn.microsoft.com/en-us/windows/win32/rstmgr/restart-manager-portal), to using [NtQueryInformationFile](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationfile).  
-   Invoke-RemoteMessage parameter Wait was changed from _bool_ to _SwitchParameter_.
  
### Removed Features
  
-   Get-LastWinSockError Cmdlet was removed. It is effectively the same as Get-LastWin32Error.