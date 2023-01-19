using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;

namespace WindowsUtils
{
    public class SystemSafeHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        override public bool IsInvalid { get; }
        public SystemSafeHandle() : base(true) { }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        override protected bool ReleaseHandle()
        {
            if (!IsInvalid && !IsClosed)
            {
                return Interop.CloseHandle(handle);
            }
            return true;
        }

        public IntPtr ToIntPtr() { return handle; }
    }

    internal class Interop
    {
        [DllImport("kernel32", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal extern static bool CloseHandle(IntPtr handle);

        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal extern static SystemSafeHandle WTSOpenServerW(string pServerName);
    }
}