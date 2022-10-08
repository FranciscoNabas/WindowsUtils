# Changelog
  
All notable changes to this project will be documented in this file.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/), from version **1.3.0** on.  
  
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