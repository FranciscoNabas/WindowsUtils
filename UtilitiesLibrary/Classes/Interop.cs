using System;
using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;
using System.Text;

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
        internal enum MouseEvent : uint
        {
            MOUSEEVENTF_LEFTDOWN = 2,
            MOUSEEVENTF_LEFTUP = 4,
            MOUSEEVENTF_RIGHTDOWN = 8,
            MOUSEEVENTF_RIGHTUP = 16
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct GUID
        {
            internal UInt32 Data1;
            internal UInt16 Data2;
            internal UInt16 Data3;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
            internal byte[] Data4;
        }

        [DllImport("kernel32", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal extern static bool CloseHandle(IntPtr handle);

        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal extern static SystemSafeHandle WTSOpenServerW(string pServerName);

        [DllImport("user32.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        internal static extern void mouse_event(
            uint dwFlags,
            uint dx,
            uint dy,
            uint cButtons,
            uint dwExtraInfo
        );
        
        [DllImport("Rstrtmgr.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern uint RmStartSession(
            out uint pSessionHandle,
            uint dwSessionFlags,
            out string strSessionKey
        );

        [DllImport("Rstrtmgr.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern uint RmEndSession(uint swSessionHandle);
    }
}
