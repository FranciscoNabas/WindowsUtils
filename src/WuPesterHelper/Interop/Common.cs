using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace WuPesterHelper.Interop;

public enum WaitResult : uint
{
    ABANDONED  = 0x00000080,
    OBJECT_0   = 0x00000000,
    TIMEOUT    = 0x00000102,
    FAILED     = 0xFFFFFFFF,
}

[Flags]
public enum AllocationType
{
    // One of these:
    MEM_COMMIT       = 0x00001000,
    MEM_RESERVE      = 0x00002000,
    MEM_RESET        = 0x00080000,
    MEM_RESET_UNDO   = 0x1000000,

    // With possibly these:
    MEM_LARGE_PAGES  = 0x20000000,
    MEM_PHYSICAL     = 0x00400000,
    MEM_TOP_DOWN     = 0x00100000
}

public enum PageProtection : uint
{
    PAGE_NOACCESS                    = 0x00000001,
    PAGE_READONLY                    = 0x00000002,
    PAGE_READWRITE                   = 0x00000004,
    PAGE_WRITECOPY                   = 0x00000008,
    PAGE_EXECUTE                     = 0x00000010,
    PAGE_EXECUTE_READ                = 0x00000020,
    PAGE_EXECUTE_READWRITE           = 0x00000040,
    PAGE_EXECUTE_WRITECOPY           = 0x00000080,
    PAGE_GUARD                       = 0x00000100,
    PAGE_NOCACHE                     = 0x00000200,
    PAGE_WRITECOMBINE                = 0x00000400,
    PAGE_GRAPHICS_NOACCESS           = 0x00000800,
    PAGE_GRAPHICS_READONLY           = 0x00001000,
    PAGE_GRAPHICS_READWRITE          = 0x00002000,
    PAGE_GRAPHICS_EXECUTE            = 0x00004000,
    PAGE_GRAPHICS_EXECUTE_READ       = 0x00008000,
    PAGE_GRAPHICS_EXECUTE_READWRITE  = 0x00010000,
    PAGE_GRAPHICS_COHERENT           = 0x00020000,
    PAGE_GRAPHICS_NOCACHE            = 0x00040000,
    PAGE_ENCLAVE_THREAD_CONTROL      = 0x80000000,
    PAGE_REVERT_TO_FILE_MAP          = 0x80000000,
    PAGE_TARGETS_NO_UPDATE           = 0x40000000,
    PAGE_TARGETS_INVALID             = 0x40000000,
    PAGE_ENCLAVE_UNVALIDATED         = 0x20000000,
    PAGE_ENCLAVE_DECOMMIT            = 0x10000000,
    PAGE_ENCLAVE_SS_FIRST            = 0x10000001,
    PAGE_ENCLAVE_SS_REST             = 0x10000002,
}

[Flags]
public enum MemFreeType : uint
{
    MEM_DECOMMIT               = 0x00004000,
    MEM_RELEASE                = 0x00008000,

    // To be used with MEM_RELEASE:
    MEM_COALESCE_PLACEHOLDERS  = 0x00000001,
    MEM_PRESERVE_PLACEHOLDER   = 0x00000002,
}

public static partial class Constants
{
    public const int MAX_PATH   = 260;
    public const uint INFINITE  = 0xFFFFFFFF;
}

public static partial class ErrorCodes
{
    public const int ERROR_SUCCESS         = 0;
    public const int ERROR_MOD_NOT_FOUND   = 126;
    public const int ERROR_ALREADY_EXISTS  = 183;
    public const int ERROR_MORE_DATA       = 234;
}

public static partial class Common
{
    [LibraryImport("kernel32.dll", SetLastError = true)]
    private static partial nint VirtualAllocEx(
        SafeProcessHandle hProcess,
        nint lpAddress,
        long dwSize,
        AllocationType flAllocationType,
        PageProtection flProtect
    );

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool VirtualFreeEx(
        SafeProcessHandle hProcess,
        nint lpAddress,
        long dwSize,
        MemFreeType dwFreeType
    );

    [LibraryImport("kernel32.dll", SetLastError = true)]
    internal static partial WaitResult WaitForSingleObject(
        SafeSystemHandle hHandle,
        uint dwMilliseconds
    );

    [LibraryImport("kernel32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf8)]
    private static partial nint GetProcAddress(
        nint hModule,
        string lpProcName
    );

    [LibraryImport("kernel32.dll", EntryPoint = "SetLastError")]
    private static partial void NativeSetLastError(uint dwErrCode);

    [LibraryImport("kernel32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "GetModuleHandleW")]
    internal static partial nint GetModuleHandle(string lpModuleName);

    [LibraryImport("kernel32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "LoadLibraryW")]
    internal static partial nint LoadLibrary(string lpFileName);

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool FreeLibrary(nint hLibModule);

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool CloseHandle(nint hObject);


    public static nint GetProcAddress(SafeModuleHandle hModule, string procName)
    {
        nint addr = GetProcAddress(hModule.DangerousGetHandle(), procName);
        if (addr == 0)
            throw new NativeException(Marshal.GetLastWin32Error());

        return addr;
    }

    public static nint VirtualAllocReadWrite(SafeProcessHandle hProcess, nint address, long size)
    {
        nint mem = VirtualAllocEx(hProcess, address, size, AllocationType.MEM_COMMIT | AllocationType.MEM_RESERVE, PageProtection.PAGE_READWRITE);
        if (mem == 0)
            throw new NativeException(Marshal.GetLastWin32Error());

        return mem;
    }

    public static void VirtualFree(SafeProcessHandle hProcess, nint address)
    {
        if (!VirtualFreeEx(hProcess, address, 0, MemFreeType.MEM_RELEASE))
            throw new NativeException(Marshal.GetLastWin32Error());
    }

    public static void SetLastError(uint errorCode)
        => NativeSetLastError(errorCode);

    public static bool IsModuleLoaded(string path)
    {
        if (!File.Exists(path))
            throw new FileNotFoundException($"Could not find file '{path}'.");

        nint hModule = GetModuleHandle(path);
        if (hModule == 0) {
            int lastError = Marshal.GetLastWin32Error();
            if (lastError != ErrorCodes.ERROR_MOD_NOT_FOUND)
                throw new NativeException(lastError);
        }

        return hModule != 0;
    }
}