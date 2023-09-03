# Changelog
  
All notable changes to this project will be documented in this file.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/), from version **1.3.0** on.  

## [1.8.2] - 2023-09-03

### Added

- Get-ObjectHandle.
  Proudly announce that `Get-ObjectHandle` now returns handles opened for registry keys, with full
  provider awareness.

### Changed

- Get-ObjectHandle.
  Improved the provider awareness both to support registry keys, and handle unsupported PS providers.

## [1.8.1] - 2023-09-01

### Added

- Start-Tcping.
  Added alias 'tcping'.

## [1.8.0] - 2023-09-01

### Added

- Start-Tcping.
  - New cmdlet that 'pings' a destination server(s) in the specified port(s).
- WuStdException.
  - Implementing `Start-Tcping` brought a lot of challenges, the most noticeable one was the performance while
    returning from functions. `WuStdException` is a C++ `std::exception` based class to slowly shift from 
    return codes to proper error handling.

### Changed

- NATIVE_CONTEXT -> WuNativeContext.
  - `WuNativeContext` is an improved version of the old `NATIVE_CONTEXT`, again to make the whole process more
  efficient and reliable.
- Shared Memory.
  - The shared memory scheme changed from using memory mapped IO to using `System.IO.UnmanagedMemoryStream`,
    which is basically a memory space to be used between Managed/Unmanaged code.
 

## [1.7.0] - 2023-08-21

The version 1.7.0 marks a turning point into this project. A lot of important things were added, and changed.
From the changes, these deserve to be highlighted.

- WindowsUtils string.
  
  This feature was one of many that came to support the `Expand-Cabinet` Cmdlet. It is a simple template string class
  based on the C++ `std::basic_string`, and the .NET `System.String`.
  I know, why create yet another string class with so many more reliable options?
  Because I wanted to, plus I got the chance to optimize it for this project. Since most allocations in the project are for strings,
  this class allows me to apply RAII to them, and manipulate them more easily.
  With this in mind, **this is probably not a good string implementation, nor does it have a reason to exist**.

- Memory allocation.
  
  A lot changed in memory allocation. With the new string class we don't need to explicitly allocate memory for strings nomo.
  The `WuMemoryManager` is also gone, all explicit allocations (where possible) where replaced with smart pointers (about damn time eh?).
  A new template expression was implemented, so we can create smart pointers with custom allocation sizes. With this I hope to reduce
  memory-related bugs from now to new versions.

- WuResult
  
  Up to version 1.6*, almost all unmanaged functions returned a `DWORD` value, much like Windows API functions.
  This was replaced (where applicable) by a new object called `WuResult`. I've implemented something similar
  in another module [LibSnitcher](https://github.com/FranciscoNabas/LibSnitcher) (that you DEFINITELY need to check out).
  I was reluctant about this kind of implementation, but the debugging benefits, and exception handling made it worth it.

- Native PSCmdlet method support.
  
  Alright, this new thing is perty damn cool. Using a system of delegates, Cmdlet context, and memory mapped IO I've implemented
  a way of calling `PSCmdlet.WriteWarning` and `PSCmdlet.WriteProgress` from 100% unmanaged code. This is also expansible for
  the other methods like `PSCmdlet.WriteError`.

  A custom command base class, inheriting from `PSCmdlet` implements native versions of `WriteWarning` and `WriteProgress`.
  This abstract class wraps a Core class which exposes the delegates that will serve as a bridge between both worlds.
  Data is exchanged between these worlds using a memory mapped file. This idea came from a video from the great **Pavel Yosifovich**
  on the subject. This approach solved all issues with string marshaling I was having before. You can find the video [here](https://www.youtube.com/watch?v=zdZdtg1f9lA).

  Unmanaged code calls a native function that maps a view of the file, writes the data, and calls a function pointer. This function pointer is
  the delegate from the Cmdlet context base class. When we wrap the command base into the C++/CLI context base, we pass our methods "casted" as
  these delegates. These methods also creates a view of the file, reads the data and calls the actual PSCmdlet methods.

### Added

- New `Expand-Cabinet` Cmdlet.
- New `Get-InstalledDotnet` function.
- Implemented `WindowsUtils.MemoryMappedShare` to exchange information between managed .NET and unmanaged C++.
- Implemented new system for writing information from unmanaged code.
  - A system using a delegate define in the Wrapper, defined in .NET, and cast to a function pointer in unmanaged code enables calling 'PSCmdlet' methods.
  - A new `CoreCommandBase` class, with a `CmdletContext` class will be responsible for carrying the context to unmanaged code. This is a very common practice in Microsoft's modules.
- Added `constexpr` routing to vector, map, and smart pointers to make the code cleaner.
- Added custom freer for smart pointers, allowing to allocate arbitrary sizes.
- New string template class `WuBaseString`, including `WWuString` for wide strings, and `WuString` for narrow strings. This is probably not a good string implementation, nor does it have
  a reason to exist, but I had fun. Plus it brings some functionalities from .NET's `System.String`.
- The new `WuResult`, and `NativeException` have a new property called `CompactTrace` exposed only in the Debug version. This contains the file name, and line where the error occurred.

### Changed

- `Remove-Service`
  - When `Stop` is used, the Cmdlet doesn't time out anymore. Instead, it writes warnings, much like when using `Stop-Service.`
- `Get-ObjectHandle`
  - The Cmdlet shows a warning when the process running is not elevated. If the process is not elevated, some handle information
    might be missing due lack of privileges to query certain processes.
- Replaced all explicit memory allocations with smart pointers. It was about time.
- Replaced all C-style strings with the new `WuString`, and its utility methods.
- `Get-FormattedMessage` changed to `Get-ErrorText`.
- `NativeException` moved to C++/CLI completely. This allowed me from removing the `NativeExceptionBase`, and creating
  exceptions from the new `WuResult` object.

### Bugs

- Calling `Get-ObjectHandle` with insufficient privileges to an object caused access violation. The function was not parsing the `NTSTATUS` from the subroutine.
- `Get-ServiceSecurity` failing to get the SDDL from a "cross-function" SECURITY_DESCRIPTOR pointer. Moved the SDDL formation to the native function.

## [1.6.2] - 2023-06-22

### Added
  
- `Get-ServiceSecurity`
  - Parameter `Name` accepts multiple names.
  - Added parameter `DisplayName`. Accepts multiple display names.
  - Added support for wildcard on `Name` and `DisplayName` parameters.
- Added tab completion for `Get-ServiceSecurity`, `Set-ServiceSecurity`, and `Remove-Service`, both on `Name` and `DisplayName` parameters.
  
### Bugs
  
- Fixed issue with ownership change when using the `Sddl` parameter with `Set-ServiceSecurity`.

## [1.6.1] - 2023-06-21

### Bugs
  
- Fixing assembly loading and dependency issues.

## [1.6.0] - 2023-06-20

### Changed
  
- WtsSession:
  - From a static class to an IDisposable class. Not only safer, but allows us to manage multiple sessions with multiple servers at the same time.
- `Get-FormattedError` renamed to `Get-ErrorString`.

### Added
  
- `Remove-Service` Cmdlet.
- `Get-ServiceSecurity` Cmdlet.
- `New-ServiceAccessRule` Cmdlet.
- `New-ServiceAuditRule` Cmdlet.
- `Set-ServiceSecurity` Cmdlet.
- `NativeException` exception on both core, and main libraries to wrap Win32 errors.
- Implemented `WindowsUtils.AccessControl.WindowsUtilsObjectSecurity` for current and future object ACL abstraction.
- Implemented `WindowsUtils.AccessControl.ServiceSecurity` to manage Windows service security.
- Added help message on all parameters.
- Added the `ValidateFileExists` attribute to validate if single files exists.
- Added `Format.ps1xml` for the new type `WindowsUtils.AccessControl.ServiceSecurity`.
- Added type support on `Types.ps1xml` for `WindowsUtils.AccessControl.ServiceSecurity`.

### Bugs
  
- Get-ResourceMessageTable:
  - "s" was being used as a format specifier, which was breaking the message strings. Replaced with "S".  
  - Trimmed output string on `Message` to remove the extra line feed.

## [1.5.1] - 2023-01-18

### Changed
  
- Structure:  
  - Rewriting the functions, removing stupid mistakes from when I was beginning with MS C++.  
  - `LocalAlloc`. `LocalAlloc` everywhere. Replacing with stack and `HeapAlloc` allocation.  
- `Get-ObjectHandle`  
  - Making getting process image version info more efficient.  
  - Abstract processes like System, Secure System and Registry now point to `ntoskrnl.exe`, like on the Task Manager.  
  - Column 'Name' added, 'Application' changed to 'Description', and property layout changed to mimic the Task Manager.  
- `Disconnect-Session`
  - Cannot use session ID 0 when disconnecting a session on a remote computer.  
  
### Added
  
- `Get-ObjectHandle`:  
  - Implement `CloseHandle`.  
  - Implemented `NtQuerySystemInformation` with `SystemProcessInformation` to cover information missing from `QueryFullProcessImageName`.  
  - New Switch Parameter `Force` to be used with `CloseHandle`. Avoids confirmation.  

### Bugs
  
- `Get-ComputerSession`  
  - When ran on the local computer, `IdleTime` should be zero for the current session. It was returning a random `filetime`. Now if `hsession = WTS_CURRENT_SERVER_HANDLE`, and the user is the running the *Cmdlet*, it returns `TimeSpan.Zero`.  
  - For system sessions, the logon time was `filetime` 0, which is 12/31/1600 10:00:00 PM. Now when `LogonTime` is `filetime` 0, it returns null.  
  
- `Get-ObjectHandle`  
  - Removing the error handling caused the protected processes not to return.
  - Using `PROCESS_QUERY_LIMITED_INFORMATION` to get version info, and image path from protected processes.
  - Processes who image doesn't have a `FileDescription`, using the image base name instead.
  - No more process missing main information, like Name and Description.

## [1.4.0] - 2022-10-12
  
### Bugs
  
- Some module functionalities depend on VC++ libraries. To avoid having to install the redistributable runtime, these assemblies are provided with the module.  
- `Invoke-RemoteMessage`:  
  - Sessions who doesn't support message box was returning the same response of the last session.  
  - When 'Wait' was called with 'Timeout' equals to zero, and there are more than one valid session on the target computer, the console would wait all session's response.  
    This causes the console to hang if there were missing responses.  
    Now, if Wait is specified, Timeout is mandatory, and an error is thrown if the user inputs zero.  

### Added Features
  
- `StageComputerSession` method added to `WtsSession` to manage persistent WTS computer sessions.  
- `Get-ObjectHandle` new alias `gethandle`.  
- New output object for `Invoke-RemoteMessage`. `MessageResponse`, with Session ID and Response.  
  
### Changed Features
  
- Release notes on the module manifest, thus also on PowerShell Gallery now points to this document.  
- `WtsSession` as a static class to use it as a 'Global Object'.  
- `Send-Click` now is called from unmanaged code using `SendInput`. Previously was called using explicit P/Invoke.  
- `Disconnect-Session` parameter `SessionId` not mandatory when disconnecting a session on the local computer. It will log off the current session.  
- All output objects are now .NET wrapper classes for the CLR ref class. This helps with cyclical dependencies, and further module expansion.  
- `Get-ObjectHandle` parameter `Path` not mandatory anymore. It will query objects in the current directory.  
- `Invoke-RemoteMessage`:  
  - Now supports should process. Previously confirmation was managed by the `UtilitiesLibrary`.  
  - Converting `SessionId` and `Response` vectors to smart pointer. Previously they were statically allocated / deallocated on the heap.  
  - Unmanaged function now returns a DWORD and receives arguments as reference. This is part of the unmanaged code, and error handling standardization.  
  - Ignoring response type 0. This is returned when the session does not support message boxes.  
  
### Removed Features
  
- `UtilitiesLibrary` was completely removed from the project. All functionalities were migrated to the module itself. The 'Wrapper' library now is called 'Core', contained in its own namespace.  
  This solves cyclical dependency issues, and helps standardize the output objects.  
  
## [1.3.4] - 2022-10-08

Version 1.3.4 improves interoperability between existing WTS *Cmdlets*.  
  
### Changed Features
  
- `Get-ComputerSession` had an additional property on its output object. `ComputerName` returns a value when the *Cmdlet* is run for a remote computer.  
  This allows the output to be passed to Disconnect-Session.  
- `Disconnect-Session` parameters `ComputerName` and `SessionId` now accepts pipeline input by property name.  
  
## [1.3.3] - 2022-10-07
  
### Changed Features
  
- `Get-FileHandle` changed to `Get-ObjectHandle`. This *Cmdlet* also works with directories, and allow future expansion to other system objects.  
- `Get-ObjectHandle` `FileName` property changed to `InputObject` to comply with the *Cmdlet* scope.  
  
### Bugs
  
- `Get-ObjectHandle` was not returning a considerable number of image properties. This was due the *Cmdlet* using a Shell interface to get properties from the files themselves.  
  This implementation was replaced by [VerQueryValue](https://learn.microsoft.com/en-us/windows/win32/api/winver/nf-winver-verqueryvaluew).  
  Besides increasing performance, the only cases where properties are not shown is when the image does not contain a resource section, or access denied to the process.  
  
## [1.3.2] - 2022-10-05
  
Version 1.3.2 fixes bugs with `Get-FileHandle` *Cmdlet*
  
### Added Features
  
- Get-FileHandle accepts wildcard.  
  
### Bugs
  
- `Get-FileHandle` crashed when receiving pipeline input from `Get-ChildItem`, only on Windows PowerShell.  
  *Cmdlet* was adapted to be "Provider-Aware". Recommendations found on [this](https://stackoverflow.com/questions/8505294/how-do-i-deal-with-paths-when-writing-a-powershell-cmdlet) question.  

## [1.3.1] - 2022-10-04
  
Version 1.3.1 fixes bugs with the PS help engine.
  
### Added Features
  
- Using *XmlDoc2CmdletDoc* for creating and managing help files.  

### Bugs
  
- Help result not showing proper parameter information, and examples in the wrong category.
  
## [1.3.0] - 2022-10-02
  
Version 1.3.0 fixes major bugs and changes features.  
  
### Added Features
  
- This project now have a *Readme*, and a *Changelog*!  
- Adhering to Semantic Versioning.

### Changed Features
  
- `Get-FileHandle` engine migrated from using the [Restart Manager](https://learn.microsoft.com/en-us/windows/win32/rstmgr/restart-manager-portal), to using [NtQueryInformationFile](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationfile).  
- I`nvoke-RemoteMessage` parameter Wait was changed from _bool_ to _SwitchParameter_.
  
### Removed Features
  
- `Get-LastWinSockError` Cmdlet was removed. It is effectively the same as Get-LastWin32Error.