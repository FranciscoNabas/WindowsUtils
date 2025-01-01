using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace WuPesterHelper.Interop;

[Flags]
public enum ProcessAccess
{
    TERMINATE                  = 0x0001,
    CREATE_THREAD              = 0x0002,
    SET_SESSIONID              = 0x0004,
    VM_OPERATION               = 0x0008,
    VM_READ                    = 0x0010,
    VM_WRITE                   = 0x0020,
    DUP_HANDLE                 = 0x0040,
    CREATE_PROCESS             = 0x0080,
    SET_QUOTA                  = 0x0100,
    SET_INFORMATION            = 0x0200,
    QUERY_INFORMATION          = 0x0400,
    SUSPEND_RESUME             = 0x0800,
    QUERY_LIMITED_INFORMATION  = 0x1000,
    SET_LIMITED_INFORMATION    = 0x2000,
    ALL_ACCESS                 = 0x001FFFFF,
}

[Flags]
public enum ThreadCreationFlags : uint
{
    NONE                               = 0x00000000,
    CREATE_SUSPENDED                   = 0x00000004,
    STACK_SIZE_PARAM_IS_A_RESERVATION  = 0x00010000,
}

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
public delegate int ThreadProc(nint lpParameter);

public static partial class Constants
{
    public const int STILL_ACTIVE = 259;
}

public static partial class NativeProcess
{
    [LibraryImport("kernel32.dll", SetLastError = true)]
    private static partial nint OpenProcess(
        ProcessAccess dwDesiredAccess,
        [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle,
        uint dwProcessId
    );

    [LibraryImport("kernel32.dll", SetLastError = true)]
    private static partial nint CreateRemoteThread(
        nint hProcess,
        nint lpThreadAttributes,
        long dwStackSize,
        nint lpStartAddress,
        nint lpParameter,
        ThreadCreationFlags dwCreationFlags,
        out int lpThreadId
    );

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool WriteProcessMemory(
        SafeProcessHandle hProcess,
        nint lpBaseAddress,
        nint lpBuffer,
        long nSize,
        out long lpNumberOfBytesWritten
    );

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool TerminateThread(
        SafeProcessHandle hThread,
        int dwExitcode
    );

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool GetExitCodeThread(
        SafeProcessHandle hThread,
        out int lpExitCode
    );

    [LibraryImport("kernel32.dll", EntryPoint = "GetCurrentProcess")]
    private static partial nint NativeGetCurrentProcess();


    public static void WriteProcessMemory(SafeProcessHandle hProcess, nint address, nint buffer, long size)
    {
        if (!WriteProcessMemory(hProcess, address, buffer, size, out _))
            throw new NativeException(Marshal.GetLastWin32Error());
    }

    public static SafeProcessHandle GetCurrentProcess()
        => new(NativeGetCurrentProcess(), false);

    public static void KillThread(SafeProcessHandle hThread, int exitCode)
    {
        if (!TerminateThread(hThread, exitCode))
            throw new NativeException(Marshal.GetLastWin32Error());
    }

    public static SafeProcessHandle OpenProcess(uint processId, ProcessAccess desiredAccess, bool inherit)
    {
        nint hProcess = OpenProcess(desiredAccess, inherit, processId);
        if (hProcess == 0)
            throw new NativeException(Marshal.GetLastWin32Error());

        return new(hProcess, true);
    }

    public static (int, SafeProcessHandle) CreateRemoteThread(SafeProcessHandle hProcess, nint threadProc, nint threadProcParam, ThreadCreationFlags flags)
    {
        nint hThread = CreateRemoteThread(hProcess.DangerousGetHandle(), nint.Zero, 0, threadProc, threadProcParam, flags, out int id);
        if (hThread == 0)
            throw new NativeException(Marshal.GetLastWin32Error());

        return (id, new(hThread, true));
    }

    public static int GetThreadExitCode(SafeProcessHandle hThread)
    {
        if (!GetExitCodeThread(hThread, out int exitCode))
            throw new NativeException(Marshal.GetLastWin32Error());

        return exitCode;
    }
}