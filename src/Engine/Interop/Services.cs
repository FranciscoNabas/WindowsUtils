using System.Runtime.InteropServices;

namespace WindowsUtils.Engine.Interop;

[Flags]
public enum ServiceRights : uint
{
    QueryConfig          = 0x0001,
    ChangeConfig         = 0x0002,
    QueryStatus          = 0x0004,
    EnumerateDependents  = 0x0008,
    Start                = 0x0010,
    Stop                 = 0x0020,
    PauseContinue        = 0x0040,
    Interrogate          = 0x0080,
    UserDefinedControl   = 0x0100,
    AllAccess            = 0xF01FF,
    Delete               = 0x10000,
    ReadControl          = 0x20000,
    WriteDac             = 0x40000,
    WriteOwner           = 0x80000,
    GenericRead          = ReadControl |
                           QueryConfig |
                           QueryStatus |
                           Interrogate |
                           EnumerateDependents,

    GenericWrite         = ReadControl | ChangeConfig,
    GenericExecute       = ReadControl |
                           Start          |
                           Stop           |
                           PauseContinue  |
                           UserDefinedControl
}

internal static class Services
{
    [DllImport("advapi32.dll", SetLastError = true)]
    internal static extern bool CloseServiceHandle(IntPtr hSCObject);
}