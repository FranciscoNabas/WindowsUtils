using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace WuPesterHelper.Interop;

public enum ScmAccess : uint
{
    CONNECT             = 0x0001,
    CREATE_SERVICE      = 0x0002,
    ENUMERATE_SERVICE   = 0x0004,
    LOCK                = 0x0008,
    QUERY_LOCK_STATUS   = 0x0010,
    MODIFY_BOOT_CONFIG  = 0x0020,
    ALL_ACCESS          = 0x000F0000         |
                          CONNECT            |
                          CREATE_SERVICE     |
                          ENUMERATE_SERVICE  |
                          LOCK               |
                          QUERY_LOCK_STATUS  |
                          MODIFY_BOOT_CONFIG,
}

public enum ServiceAccess : uint
{
    QUERY_CONFIG          = 0x0001,
    CHANGE_CONFIG         = 0x0002,
    QUERY_STATUS          = 0x0004,
    ENUMERATE_DEPENDENTS  = 0x0008,
    START                 = 0x0010,
    STOP                  = 0x0020,
    PAUSE_CONTINUE        = 0x0040,
    INTERROGATE           = 0x0080,
    USER_DEFINED_CONTROL  = 0x0100,
    ALL_ACCESS            = 0x000F0000            |
                            QUERY_CONFIG          |
                            CHANGE_CONFIG         |
                            QUERY_STATUS          |
                            ENUMERATE_DEPENDENTS  |
                            START                 |
                            STOP                  |
                            PAUSE_CONTINUE        |
                            INTERROGATE           |
                            USER_DEFINED_CONTROL,
}

public enum ServiceType : uint
{
    KERNEL_DRIVER         = 0x00000001,
    FILE_SYSTEM_DRIVER    = 0x00000002,
    ADAPTER               = 0x00000004,
    RECOGNIZER_DRIVER     = 0x00000008,
    DRIVER                = KERNEL_DRIVER | FILE_SYSTEM_DRIVER | RECOGNIZER_DRIVER,
    WIN32_OWN_PROCESS     = 0x00000010,
    WIN32_SHARE_PROCESS   = 0x00000020,
    WIN32                 = WIN32_OWN_PROCESS | WIN32_SHARE_PROCESS,
    USER_SERVICE          = 0x00000040,
    USERSERVICE_INSTANCE  = 0x00000080,
    USER_SHARE_PROCESS    = USER_SERVICE | WIN32_SHARE_PROCESS,
    USER_OWN_PROCESS      = USER_SERVICE | WIN32_OWN_PROCESS,
    INTERACTIVE_PROCESS   = 0x00000100,
    PKG_SERVICE           = 0x00000200,
    ALL                   = WIN32 | ADAPTER | DRIVER | INTERACTIVE_PROCESS |
                            USER_SERVICE | USERSERVICE_INSTANCE | PKG_SERVICE,
}

public enum ServiceState : uint
{
    STOPPED           = 0x00000001,
    START_PENDING     = 0x00000002,
    STOP_PENDING      = 0x00000003,
    RUNNING           = 0x00000004,
    CONTINUE_PENDING  = 0x00000005,
    PAUSE_PENDING     = 0x00000006,
    PAUSED            = 0x00000007,
}

[Flags]
public enum ServiceControl : uint
{
    STOP                   = 0x00000001,
    PAUSE_CONTINUE         = 0x00000002,
    SHUTDOWN               = 0x00000004,
    PARAMCHANGE            = 0x00000008,
    NETBINDCHANGE          = 0x00000010,
    HARDWAREPROFILECHANGE  = 0x00000020,
    POWEREVENT             = 0x00000040,
    SESSIONCHANGE          = 0x00000080,
    PRESHUTDOWN            = 0x00000100,
    TIMECHANGE             = 0x00000200,
    TRIGGEREVENT           = 0x00000400,
    USER_LOGOFF            = 0x00000800,
    LOWRESOURCES           = 0x00002000,
    SYSTEMLOWRESOURCES     = 0x00004000,
}

public enum ServiceStartType : uint
{
    BOOT_START    = 0x00000000,
    SYSTEM_START  = 0x00000001,
    AUTO_START    = 0x00000002,
    DEMAND_START  = 0x00000003,
    DISABLED      = 0x00000004,
}

public enum ServiceErrorControl : uint
{
    IGNORE    = 0x00000000,
    NORMAL    = 0x00000001,
    SEVERE    = 0x00000002,
    CRITICAL  = 0x00000003,
}

public enum ServiceEnumState : uint
{
    ACTIVE    = 0x00000001,
    INACTIVE  = 0x00000002,
    ALL       = 0x00000003
}

public enum ServiceFlags : uint
{
    NONE = 0x00000000,
    SERVICE_RUNS_IN_SYSTEM_PROCESS = 0x00000001,
}

public enum SC_STATUS_TYPE
{
    PROCESS_INFO = 0,
}

public enum SC_ENUM_TYPE
{
    PROCESS_INFO = 0,
}

[StructLayout(LayoutKind.Sequential)]
public struct SERVICE_STATUS
{
    public ServiceType dwServiceType;
    public ServiceState dwCurrentState;
    public ServiceControl dwControlsAccepted;
    public uint dwWin32ExitCode;
    public uint dwServiceSpecificExitCode;
    public uint dwCheckPoint;
    public uint dwWaitHint;
}

[StructLayout(LayoutKind.Sequential)]
public struct SERVICE_STATUS_PROCESS
{
    public ServiceType dwServiceType;
    public ServiceState dwCurrentState;
    public ServiceControl dwControlsAccepted;
    public uint dwWin32ExitCode;
    public uint dwServiceSpecificExitCode;
    public uint dwCheckPoint;
    public uint dwWaitHint;
    public uint dwProcessId;
    public ServiceFlags dwServiceFlags;
}

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
public struct ENUM_SERVICE_STATUS
{
    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpServiceName;

    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpDisplayName;
    public SERVICE_STATUS ServiceStatus;
}

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
public struct QUERY_SERVICE_CONFIG
{
    public ServiceType dwServiceType;
    public ServiceStartType dwStartType;
    public ServiceErrorControl dwErrorControl;

    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpBinaryPathName;

    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpLoadOrderGroup;
    public uint dwTagId;

    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpDependencies;

    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpServiceStartName;

    [MarshalAs(UnmanagedType.LPWStr)]
    public string lpDisplayName;
}

public static partial class Constants
{
    public static readonly string SERVICES_ACTIVE_DATABASE = "ServicesActive";
}

public static partial class NativeServices
{
    [LibraryImport("advapi32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "OpenSCManagerW")]
    internal static partial nint OpenSCManager(
        string lpMachineName,
        string lpDatabaseName,
        ScmAccess dwDesiredAccess
    );

    [LibraryImport("advapi32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "OpenServiceW")]
    internal static partial nint OpenService(
        nint hSCManager,
        string lpServiceName,
        ServiceAccess dwDesiredAccess
    );

    [LibraryImport("advapi32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "CreateServiceW")]
    private static partial nint CreateService(
        nint hSCManager,
        string lpServiceName,
        string lpDisplayName,
        ServiceAccess dwDesiredAccess,
        ServiceType dwServiceType,
        ServiceStartType dwStartType,
        ServiceErrorControl dwErrorControl,
        string lpBinaryPathName,
        string lpLoadOrderGroup,
        nint lpdwTagId,
        string lpDependencies,
        string lpServiceStartName,
        string lpPassword
    );

    [LibraryImport("advapi32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool QueryServiceStatusEx(
        nint hService,
        SC_STATUS_TYPE InfoLevel,
        ref SERVICE_STATUS_PROCESS lpBuffer,
        int cbBufSize,
        out int pcbBytesNeeded
    );

    [LibraryImport("advapi32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "EnumDependentServicesW")]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool EnumDependentServices(
        nint hService,
        ServiceEnumState dwServiceState,
        nint lpServices,
        int cbBufSize,
        out int pcbBytesNeeded,
        out int lpServicesReturned
    );

    [LibraryImport("advapi32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16, EntryPoint = "StartServiceW")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool StartService(
        nint hService,
        int dwNumServiceArgs,
        ref string lpServiceArgVectors
    );

    [LibraryImport("advapi32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool ControlService(
        nint hService,
        ServiceControl dwControl,
        ref SERVICE_STATUS lpServiceStatus
    );

    [LibraryImport("advapi32.dll", SetLastError = true, EntryPoint = "DeleteService")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool NativeDeleteService(nint hService);

    [LibraryImport("advapi32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static partial bool CloseServiceHandle(nint hSCObject);

#pragma warning disable CS8604
    public static ManagedService CreateService(string name, string displayName, string binaryPath, string computerName = "", string[]? dependencies = null, ServiceAccess desiredAccess = ServiceAccess.ALL_ACCESS,
        ServiceType type = ServiceType.WIN32_OWN_PROCESS, ServiceStartType startType = ServiceStartType.DEMAND_START, ServiceErrorControl errorControl = ServiceErrorControl.NORMAL,
        string loadOrderGroup = "", string? account = null, string? password = null)
    {
        StringBuilder depBuffer = new();
        if (dependencies is not null && dependencies.Length > 0) {
            foreach (string dependency in dependencies) {
                depBuffer.Append($"{dependency}\0");
            }

            depBuffer.Append("\0\0");
        }

        binaryPath = $"\"{binaryPath}\"";

        nint hScm = OpenSCManager(computerName, Constants.SERVICES_ACTIVE_DATABASE, ScmAccess.CONNECT | ScmAccess.CREATE_SERVICE);
        if (hScm == 0)
            throw new NativeException(Marshal.GetLastWin32Error());

        nint hService = 0;
        try {
            hService = CreateService(hScm, name, displayName, desiredAccess, type, startType, errorControl, binaryPath, loadOrderGroup, 0, depBuffer.ToString(), account, password);
            if (hService == 0)
                throw new NativeException(Marshal.GetLastWin32Error());
        }
        catch {
            CloseServiceHandle(hScm);
        }

        return new(name, computerName, hScm, hService);
    }
#pragma warning restore CS8604

    internal static void StartService(string name, nint hService, string[]? args)
    {
        StringBuilder argsBuffer = new();
        if (args is not null && args.Length > 0) {
            argsBuffer.Append($"{name}\0");
            foreach (string arg in args) {
                argsBuffer.Append($"{arg}\0");
            }

            argsBuffer.Append("\0\0");
        }

        string argsStr = argsBuffer.ToString();
        if (!StartService(hService, args is null ? 0 : args.Length, ref argsStr))
            throw new NativeException(Marshal.GetLastWin32Error());
    }

    internal static void GetServiceStatus(nint hService, ref SERVICE_STATUS_PROCESS status)
    {
        int buffSize = Marshal.SizeOf<SERVICE_STATUS_PROCESS>();
        if (!QueryServiceStatusEx(hService, SC_STATUS_TYPE.PROCESS_INFO, ref status, buffSize, out _))
            throw new NativeException(Marshal.GetLastWin32Error());
    }

    internal static SERVICE_STATUS ControlService(nint hService, ServiceControl control)
    {
        SERVICE_STATUS status = new();
        if (!ControlService(hService, control, ref status))
            throw new NativeException(Marshal.GetLastWin32Error());

        return status;
    }

    internal static List<string> GetActiveDependentServices(nint hService)
    {
        List<string> output = [];
        if (EnumDependentServices(hService, ServiceEnumState.ACTIVE, 0, 0, out int bytesNeeded, out _))
            return output;

        int lastError = Marshal.GetLastWin32Error();
        if (lastError != ErrorCodes.ERROR_MORE_DATA)
            throw new NativeException(lastError);

        nint buffer = Marshal.AllocHGlobal(bytesNeeded);
        try {
            if (!EnumDependentServices(hService, ServiceEnumState.ACTIVE, buffer, bytesNeeded, out bytesNeeded, out int count))
                throw new NativeException(lastError);

            int dataSize = Marshal.SizeOf<ENUM_SERVICE_STATUS>();
            for (int i = 0; i < count; i++) {
                output.Add(Marshal.PtrToStructure<ENUM_SERVICE_STATUS>(buffer).lpServiceName);
                buffer = nint.Add(buffer, dataSize);
            }
        }
        finally {
            Marshal.FreeHGlobal(buffer);
        }

        return output;
    }

    internal static void DeleteService(nint hService)
    {
        if (!NativeDeleteService(hService))
            throw new NativeException(Marshal.GetLastWin32Error());
    }
}