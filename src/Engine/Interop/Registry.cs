using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using Microsoft.Win32;

namespace WindowsUtils.Engine.Interop;

internal enum KEY_INFORMATION_CLASS
{
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    KeyTrustInformation,
    KeyLayerInformation,
    MaxKeyInfoClass
}

[StructLayout(LayoutKind.Sequential)]
internal struct KEY_NAME_INFORMATION
{
    internal int NameLength;
    private readonly char m_name;

    internal readonly string Name => GetName();

    private unsafe readonly string GetName()
    {
        fixed (char* namePtr = &m_name) {
            return new string(namePtr, 0, NameLength);
        }
    }
}

internal static class NativeRegistry
{
    [DllImport("ntdll.dll", SetLastError = true)]
    private static extern int NtQueryKey(
        IntPtr KeyHandle,
        KEY_INFORMATION_CLASS KeyInformationClass,
        IntPtr KeyInformation,
        int Length,
        out int ResultLength
    );

    [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "RegConnectRegistryW")]
    private static extern int RegConnectRegistry(
        IntPtr lpMachineName,
        RegistryHive hKey,
        out SafeRegistryHandle phkResult
    );

    internal static SafeRegistryHandle ConnectRegistry(string? computerName, RegistryHive hive)
    {
        IntPtr pcNameBuffer = computerName is null ? IntPtr.Zero : Marshal.StringToHGlobalUni(computerName);
        try {
            int result = RegConnectRegistry(pcNameBuffer, hive, out SafeRegistryHandle handle);
            if (result != 0)
                throw new NativeException(result);
     
            return handle;
        }
        finally {
            if (pcNameBuffer != IntPtr.Zero)
                Marshal.FreeHGlobal(pcNameBuffer);
        }
    }
}