# Changelog
  
All notable changes to this project will be documented in this file.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/), from version **1.3.0** on.  
  
# [1.3.0] - 2022-10-02
  
Version 1.3.0 fixes major bugs and changes features.  
  
## Added Features
  
-   This project now have a Readme, and a Changelog!  
-   Adhering to Semantic Versioning.

## Changed Features
  
-   **Get-FileHandle** engine migrated from using the [Restart Manager](https://learn.microsoft.com/en-us/windows/win32/rstmgr/restart-manager-portal), to using [NtQueryInformationFile](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationfile).  
-   **Invoke-RemoteMessage** parameter **Wait** was changed from _bool_ to _SwitchParameter_.
  
## Removed Features
  
-   **Get-LastWinSockError** Cmdlet was removed. It is effectively the same as **Get-LastWin32Error**.