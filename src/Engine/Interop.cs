using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;
using System.Text;

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

    internal enum SID_NAME_USE
    {
        SidTypeUser = 1,
        SidTypeGroup,
        SidTypeDomain,
        SidTypeAlias,
        SidTypeWellKnownGroup,
        SidTypeDeletedAccount,
        SidTypeInvalid,
        SidTypeUnknown,
        SidTypeComputer
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
        internal const int ERROR_INSUFFICIENT_BUFFER = 122;
        internal const int ERROR_INVALID_FLAGS = 1004;
        internal const int ERROR_SUCCESS = 0;
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

        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        internal static extern bool LookupAccountName(
            string lpSystemName,
            string lpAccountName,
            [MarshalAs(UnmanagedType.LPArray)] byte[] Sid,
            ref uint cbSid,
            StringBuilder ReferencedDomainName,
            ref uint cchReferencedDomainName,
            out SID_NAME_USE peUse
        );

        [DllImport("advapi32", CharSet = CharSet.Auto, SetLastError = true)]
        internal static extern bool CheckTokenMembership(
            IntPtr tokenHandle,
            [MarshalAs(UnmanagedType.LPArray)] byte[] Sid,
            out bool isMember
        );

        [DllImport("kernel32", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal static extern bool CloseHandle(IntPtr handle);
    }
}