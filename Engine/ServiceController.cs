using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using WindowsUtils.Core;
using WindowsUtils.AccessControl;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("Advapi32.dll", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal static extern bool CloseServiceHandle(IntPtr hSCObject);
    }
}

namespace WindowsUtils.Engine
{
    [Flags]
    public enum ServiceRights : uint
    {
        QueryConfig = 0x0001,
        ChangeConfig = 0x0002,
        QueryStatus = 0x0004,
        EnumerateDependents = 0x0008,
        Start = 0x0010,
        Stop = 0x0020,
        PauseContinue = 0x0040,
        Interrogate = 0x0080,
        UserDefinedControl = 0x0100,
        AllAccess = 0xF01FF,
        AccessSystemSecurity = 0x01000000,
        Delete = 0x10000,
        ReadControl = 0x20000,
        WriteDac = 0x40000,
        WriteOwner = 0x80000,
        GenericRead = ReadControl |
                       QueryConfig |
                       QueryStatus |
                       Interrogate |
                       EnumerateDependents,

        GenericWrite = ReadControl | ChangeConfig,
        GenericExecute = ReadControl |
                          Start |
                          Stop |
                          PauseContinue |
                          UserDefinedControl
    }


    public sealed class ServiceController
    {
        private static readonly WrapperFunctions _unwrapper = new();

        public static ServiceSecurity GetServiceObjectSecurity(string serviceName)
        {
            byte[] securityDescriptorBytes;
            try
            {
                securityDescriptorBytes = _unwrapper.GetServiceSecurityDescriptorBytes(serviceName);
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }

            return new ServiceSecurity(securityDescriptorBytes);
        }
    }
}