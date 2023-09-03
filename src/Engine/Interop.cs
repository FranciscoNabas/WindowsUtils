using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;

namespace WindowsUtils.Interop
{
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
    internal struct LARGE_INTEGER
    {
        internal uint LowPart;
        internal int HighPart;
        internal long QuadPart;

        internal LARGE_INTEGER(long quadPart)
            => (LowPart, HighPart, QuadPart) =
                ((uint)(quadPart & 0xFFFFFFFF), (int)(quadPart & unchecked((long)0xFFFFFFFF00000000)), quadPart);

        #pragma warning disable CS0675
        internal LARGE_INTEGER(uint lowPart, int highPart)
            => (LowPart, HighPart, QuadPart) =
                (lowPart, highPart, (highPart << 32) | lowPart);
        #pragma warning restore CS0675

        internal void SetQuadPart(long quadPart)
            => (LowPart, HighPart, QuadPart) =
                ((uint)(quadPart & 0xFFFFFFFF), (int)(quadPart & unchecked((long)0xFFFFFFFF00000000)), quadPart);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LARGE_UINTEGER
    {
        internal uint LowPart;
        internal uint HighPart;
        internal ulong QuadPart;

        internal LARGE_UINTEGER(ulong quadPart)
            => (LowPart, HighPart, QuadPart) =
                ((uint)(quadPart & 0xFFFFFFFF), (uint)(quadPart & 0xFFFFFFFF00000000), quadPart);

        internal LARGE_UINTEGER(uint lowPart, uint highPart)
            => (LowPart, HighPart, QuadPart) =
                (lowPart, highPart, (highPart << 32) | lowPart);

        internal void SetQuadPart(ulong quadPart)
            => (LowPart, HighPart, QuadPart) =
                ((uint)(quadPart & 0xFFFFFFFF), (uint)(quadPart & 0xFFFFFFFF00000000), quadPart);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct KEY_NAME_INFORMATION
    {
        internal uint NameLength;

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Name;
    }

    public class SafeSystemHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public SafeSystemHandle() : base(true) { }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        override protected bool ReleaseHandle()
        {
            if (!IsInvalid && !IsClosed)
            {
                return NativeFunctions.CloseHandle(handle);
            }
            return true;
        }
    }

    public class SafeServiceHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public SafeServiceHandle() : base(true) { }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        override protected bool ReleaseHandle() => NativeFunctions.CloseServiceHandle(handle);
    }

    internal partial class NativeConstants
    {
        internal static readonly IntPtr INVALID_HANDLE_VALUE = (IntPtr)(-1);
    }

    internal partial class NativeFunctions
    {
        [DllImport("ntdll.dll", SetLastError = true)]
        internal static extern int NtQueryKey(
            IntPtr KeyHandle,
            KEY_INFORMATION_CLASS KeyInformationClass,
            IntPtr KeyInformation,
            int Length,
            out int ResultLength
        );

        [DllImport("kernel32", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal static extern bool CloseHandle(IntPtr handle);
    }
}