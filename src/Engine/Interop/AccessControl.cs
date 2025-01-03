using System.Runtime.InteropServices;
using System.Text;

namespace WindowsUtils.Engine.Interop;

#region Enumerations

[Flags]
internal enum AccessType : uint
{
    DELETE                    = 0x00010000,
    READ_CONTROL              = 0x00020000,
    WRITE_DAC                 = 0x00040000,
    WRITE_OWNER               = 0x00080000,
    SYNCHRONIZE               = 0x00100000,
    STANDARD_RIGHTS_REQUIRED  = 0x000F0000,
    STANDARD_RIGHTS_READ      = READ_CONTROL,
    STANDARD_RIGHTS_WRITE     = READ_CONTROL,
    STANDARD_RIGHTS_EXECUTE   = READ_CONTROL,
    STANDARD_RIGHTS_ALL       = 0x001F0000,
    SPECIFIC_RIGHTS_ALL       = 0x0000FFFF,
    ACCESS_SYSTEM_SECURITY    = 0x01000000,
    MAXIMUM_ALLOWED           = 0x02000000,
    GENERIC_READ              = 0x80000000,
    GENERIC_WRITE             = 0x40000000,
    GENERIC_EXECUTE           = 0x20000000,
    GENERIC_ALL               = 0x10000000
}

[Flags]
internal enum TokenAccessRight : uint
{
    ASSIGN_PRIMARY             = 0x0001,
    DUPLICATE                  = 0x0002,
    IMPERSONATE                = 0x0004,
    QUERY                      = 0x0008,
    QUERY_SOURCE               = 0x0010,
    ADJUST_PRIVILEGES          = 0x0020,
    ADJUST_GROUPS              = 0x0040,
    ADJUST_DEFAULT             = 0x0080,
    ADJUST_SESSIONID           = 0x0100,
    ALL_ACCESS_P               = AccessType.STANDARD_RIGHTS_REQUIRED  |
                                 ASSIGN_PRIMARY                       |
                                 DUPLICATE                            |
                                 IMPERSONATE                          |
                                 QUERY                                |
                                 QUERY_SOURCE                         |
                                 ADJUST_PRIVILEGES                    |
                                 ADJUST_GROUPS                        |
                                 ADJUST_DEFAULT,
    
    ALL_ACCESS                 = ALL_ACCESS_P | ADJUST_SESSIONID,  // ((defined(_WIN32_WINNT) && (_WIN32_WINNT > 0x0400)) || (!defined(_WIN32_WINNT)))
    READ                       = AccessType.STANDARD_RIGHTS_READ | QUERY,
    WRITE                      = AccessType.STANDARD_RIGHTS_WRITE  |
                                 ADJUST_PRIVILEGES                 |
                                 ADJUST_GROUPS                     |
                                 ADJUST_DEFAULT,

    EXECUTE                    = AccessType.STANDARD_RIGHTS_EXECUTE,
    TRUST_CONSTRAINT_MASK      = AccessType.STANDARD_RIGHTS_READ  |
                                 QUERY                            |
                                 QUERY_SOURCE,

    TRUST_ALLOWED_MASK         = TRUST_CONSTRAINT_MASK  |
                                 DUPLICATE              |
                                 IMPERSONATE,

    ACCESS_PSEUDO_HANDLE_WIN8  = QUERY | QUERY_SOURCE,       // (NTDDI_VERSION >= NTDDI_WIN8)
    ACCESS_PSEUDO_HANDLE       = ACCESS_PSEUDO_HANDLE_WIN8,  // (NTDDI_VERSION >= NTDDI_WIN8)
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

#endregion

#region Structures

[StructLayout(LayoutKind.Sequential)]
internal struct SECURITY_ATTRIBUTES
{
    internal uint nLength;
    internal IntPtr lpSecurityDescriptor;

    [MarshalAs(UnmanagedType.Bool)]
    internal bool bInheritHandle;
}

#endregion

internal static class NativeAccessControl
{
    [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "LookupAccountNameW")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern unsafe bool LookupAccountName(
        char* lpSystemName,
        char* lpAccountName,
        void* Sid,
        int* cbSid,
        char* ReferencedDomainName,
        int* cchReferencedDomainName,
        SID_NAME_USE* peUse
    );

    [DllImport("advapi32", SetLastError = true)]
    private static extern bool CheckTokenMembership(
        IntPtr tokenHandle,
        IntPtr Sid,
        out bool isMember
    );

    [DllImport("advapi32.dll", SetLastError = true, EntryPoint = "RevertToSelf")]
    private static extern bool NativeRevertToSelf();

    internal static unsafe ScopedBuffer GetAccountSid(string? computerName, string accountName)
    {
        int sidSize = 0;
        int refDomSize = 0;
        SID_NAME_USE sidUse = SID_NAME_USE.SidTypeUser;
        if (computerName is null) {
            fixed (char* namePtr = accountName) {
                LookupAccountName(null, namePtr, null, &sidSize, null, &refDomSize, &sidUse);

                ScopedBuffer sid = new(sidSize);
                char* refDomain = stackalloc char[refDomSize];
                if (!LookupAccountName(null, namePtr, sid, &sidSize, refDomain, &refDomSize, &sidUse)) {
                    sid.Dispose();
                    throw new NativeException(Marshal.GetLastWin32Error());
                }

                return sid;
            }
        }
        else {
            fixed (char* pcNamePtr = computerName)
            fixed (char* namePtr = accountName) {
                LookupAccountName(pcNamePtr, namePtr, null, &sidSize, null, &refDomSize, &sidUse);

                ScopedBuffer sid = new(sidSize);
                Span<char> refDomain = stackalloc char[refDomSize];
                fixed (char* refDomPtr = &MemoryMarshal.GetReference(refDomain)) {
                    if (!LookupAccountName(pcNamePtr, namePtr, sid, &sidSize, refDomPtr, &refDomSize, &sidUse)) {
                        sid.Dispose();
                        throw new NativeException(Marshal.GetLastWin32Error());
                    }
                }

                return sid;
            }
        }
    }

    internal static bool CheckCurrentTokenMembership(ScopedBuffer sid)
    {
        if (!CheckTokenMembership(IntPtr.Zero, sid, out bool isMember))
            throw new NativeException(Marshal.GetLastWin32Error());

        return isMember;
    }

    internal static void RevertToSelf()
    {
        if (!NativeRevertToSelf())
            throw new NativeException(Marshal.GetLastWin32Error());
    }
}