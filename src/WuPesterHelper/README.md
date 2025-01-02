# WindowsUtils Pester Test Helper Library

This library contains objects and methods to aid in testing the `WindowsUtils` module using `Pester`.  
It was designed to be imported to PowerShell or Windows PowerShell using the `Add-Type` Cmdlet, or the reflection APIs.

## Helper APIs

- Set the last error using `SetLastError`.
- Load modules in the current or external process.
- Manage services in the current and remote computer. Create, delete, set security, set properties, etc.
- Create TCP and UDP endpoints with different ports.
